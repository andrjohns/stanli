#!/usr/bin/env python3
"""Run the developer-only ODE ceiling matrix in isolated processes.

Each manifest row gets a fresh benchmark process so executor capacities and
Stan Math arena state cannot leak between shapes.  Results are appended to
JSONL as they finish; a timeout or malformed benchmark result is recorded and
does not stop the remaining cases.

This is screening infrastructure.  Low-iteration results locate crossovers;
admission decisions still require longer paired runs of the selected cases.
"""

import argparse
import datetime
import hashlib
import json
import math
import os
import pathlib
import platform
import re
import shlex
import shutil
import socket
import stat
import subprocess
import sys
import tempfile
import time


SOLVERS = ("rk45", "ckrk", "bdf", "adams")
RK_SOLVERS = frozenset(("rk45", "ckrk"))
KV_RE = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)=(?:\"([^\"]*)\"|(\S+))")
LOG_SENTINEL = ".stanli-ode-ceiling-logs-v1"
LOG_SENTINEL_CONTENT = "stanli ODE ceiling sweep log directory v1\n"
PROVENANCE_SUFFIX = ".provenance.json"
CMAKE_CACHE_KEYS = frozenset(
    (
        "CMAKE_BUILD_TYPE",
        "CMAKE_C_COMPILER",
        "CMAKE_C_COMPILER_LAUNCHER",
        "CMAKE_C_FLAGS",
        "CMAKE_CXX_COMPILER",
        "CMAKE_CXX_COMPILER_LAUNCHER",
        "CMAKE_CXX_EXTENSIONS",
        "CMAKE_CXX_FLAGS",
        "CMAKE_CXX_STANDARD",
        "CMAKE_EXE_LINKER_FLAGS",
        "CMAKE_INTERPROCEDURAL_OPTIMIZATION",
        "CMAKE_SHARED_LINKER_FLAGS",
        "CMAKE_STATIC_LINKER_FLAGS",
    )
)


def absolute_path(path):
    """Make a path absolute without hiding a symlink at the leaf."""
    return pathlib.Path(os.path.abspath(os.fspath(path.expanduser())))


def canonical_path(path):
    """Resolve parent symlinks for collision checks without opening a file."""
    return pathlib.Path(os.path.realpath(os.fspath(path)))


def reject_output_collisions(output, log_dir, provenance_path, protected):
    destinations = {
        canonical_path(output): "output",
        canonical_path(provenance_path): "provenance sidecar",
    }
    canonical_log_dir = canonical_path(log_dir)
    for source in protected:
        canonical_source = canonical_path(source)
        if canonical_source in destinations:
            raise ValueError(
                f"refusing {destinations[canonical_source]} collision with "
                f"benchmark input: {source}"
            )
        if (canonical_source == canonical_log_dir or
                canonical_log_dir in canonical_source.parents):
            raise ValueError(
                f"refusing log directory containing benchmark input: "
                f"{source}"
            )


def path_exists(path):
    """Like lexists(3): broken symlinks count as existing paths."""
    return path.exists() or path.is_symlink()


def require_regular_file(path, label):
    if path.is_symlink():
        raise ValueError(f"refusing symlink {label}: {path}")
    if not stat.S_ISREG(path.lstat().st_mode):
        raise ValueError(f"expected regular {label}: {path}")


def owned_log_files(log_dir):
    """Validate an existing log directory before --force removes it."""
    if log_dir.is_symlink():
        raise ValueError(f"refusing symlink log directory: {log_dir}")
    if not log_dir.is_dir():
        raise ValueError(f"expected log directory: {log_dir}")
    sentinel = log_dir / LOG_SENTINEL
    if not path_exists(sentinel):
        raise ValueError(
            f"refusing to clean unowned log directory without {LOG_SENTINEL}: "
            f"{log_dir}"
        )
    require_regular_file(sentinel, "log-directory sentinel")
    if sentinel.read_text() != LOG_SENTINEL_CONTENT:
        raise ValueError(f"refusing log directory with invalid sentinel: {log_dir}")
    logs = []
    for child in log_dir.iterdir():
        if child == sentinel:
            continue
        if child.is_symlink() or child.suffix != ".log":
            raise ValueError(
                f"refusing log directory with unexpected entry: {child}"
            )
        require_regular_file(child, "log file")
        logs.append(child)
    return sentinel, logs


def prepare_output_paths(output, log_dir, provenance_path, force):
    """Create an empty, owned output area without broad deletion."""
    for path, label in (
        (output, "output"),
        (log_dir, "log directory"),
        (provenance_path, "provenance sidecar"),
    ):
        if path.is_symlink():
            raise ValueError(f"refusing symlink {label}: {path}")

    existing = [
        path for path in (output, log_dir, provenance_path) if path_exists(path)
    ]
    if existing and not force:
        raise FileExistsError(
            "refusing to replace existing artifact(s): "
            + ", ".join(str(path) for path in existing)
            + "; pass --force"
        )

    log_cleanup = None
    if path_exists(log_dir):
        log_cleanup = owned_log_files(log_dir)
    for path, label in (
        (output, "output"),
        (provenance_path, "provenance sidecar"),
    ):
        if path_exists(path):
            require_regular_file(path, label)

    # All existing artifacts have been validated before removing any of them.
    if force:
        for path in (output, provenance_path):
            if path_exists(path):
                path.unlink()
        if log_cleanup is not None:
            sentinel, logs = log_cleanup
            for child in logs:
                child.unlink()
            sentinel.unlink()
            log_dir.rmdir()

    output.parent.mkdir(parents=True, exist_ok=True)
    log_dir.mkdir()
    with (log_dir / LOG_SENTINEL).open("x") as stream:
        stream.write(LOG_SENTINEL_CONTENT)


def sha256_file(path, cache=None):
    key = str(path)
    if cache is not None and key in cache:
        return cache[key]
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    value = digest.hexdigest()
    if cache is not None:
        cache[key] = value
    return value


def current_case_hashes(case_record, executable):
    return {
        "mir": sha256_file(case_record["mir"]),
        "data": sha256_file(case_record["data"]),
        "selected_binary": sha256_file(executable),
    }


def case_hash_error(case_record, executable, expected):
    try:
        observed = current_case_hashes(case_record, executable)
    except OSError as error:
        return f"benchmark input became unreadable: {error}"
    changed = [
        name for name, digest in expected.items()
        if observed.get(name) != digest
    ]
    if changed:
        return "benchmark input hash changed: " + ", ".join(changed)
    return None


def command_output(command, cwd=None, timeout=10.0):
    try:
        completed = subprocess.run(
            command,
            cwd=cwd,
            capture_output=True,
            text=True,
            timeout=timeout,
            check=False,
        )
    except (OSError, subprocess.TimeoutExpired) as error:
        return {"ok": False, "error": str(error)}
    output = (completed.stdout or completed.stderr).strip()
    return {
        "ok": completed.returncode == 0,
        "returncode": completed.returncode,
        "output": output,
    }


def git_provenance(cwd):
    root_result = command_output(
        ["git", "rev-parse", "--show-toplevel"], cwd=cwd
    )
    if not root_result["ok"]:
        return {
            "available": False,
            "error": root_result.get("error") or root_result.get("output", ""),
        }
    root = pathlib.Path(root_result["output"])
    head = command_output(["git", "rev-parse", "HEAD"], cwd=root)
    branch = command_output(
        ["git", "branch", "--show-current"], cwd=root
    )
    status_result = command_output(
        ["git", "status", "--porcelain=v1", "--untracked-files=normal"],
        cwd=root,
    )
    status_lines = (
        status_result.get("output", "").splitlines()
        if status_result["ok"] else []
    )
    return {
        "available": True,
        "root": str(root),
        "head": head.get("output") if head["ok"] else None,
        "branch": branch.get("output") if branch["ok"] else None,
        "dirty": bool(status_lines) if status_result["ok"] else None,
        "status_entry_count": len(status_lines) if status_result["ok"] else None,
        "status": status_lines if status_result["ok"] else None,
        "status_error": None if status_result["ok"] else status_result,
    }


def find_cmake_cache(executable):
    for directory in (executable.parent, *executable.parents):
        candidate = directory / "CMakeCache.txt"
        if candidate.is_file() and not candidate.is_symlink():
            return candidate
    return None


def relevant_cmake_cache(cache_path):
    entries = {}
    for line in cache_path.read_text(errors="replace").splitlines():
        if not line or line.startswith(("#", "//")) or "=" not in line:
            continue
        declaration, value = line.split("=", 1)
        key = declaration.split(":", 1)[0]
        if (
            key in CMAKE_CACHE_KEYS
            or key.startswith("CMAKE_C_FLAGS_")
            or key.startswith("CMAKE_CXX_FLAGS_")
            or key.startswith("CMAKE_EXE_LINKER_FLAGS_")
            or key.startswith("CMAKE_SHARED_LINKER_FLAGS_")
            or key.startswith("CMAKE_STATIC_LINKER_FLAGS_")
        ):
            entries[key] = value
    return entries


def target_compile_flags(executable):
    cache_path = find_cmake_cache(executable)
    if cache_path is None:
        return None
    flags_path = (
        cache_path.parent / "CMakeFiles" / f"{executable.name}.dir" / "flags.make"
    )
    if flags_path.is_symlink() or not flags_path.is_file():
        return None
    assignments = {}
    for line in flags_path.read_text(errors="replace").splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        key = key.strip()
        if key.startswith(("CXX_FLAGS", "CXX_DEFINES")):
            assignments[key] = value.strip()
    return {
        "path": str(flags_path),
        "sha256": sha256_file(flags_path),
        "assignments": assignments,
    }


def compiler_provenance(cmake_records):
    candidates = []
    for record in cmake_records:
        entries = record["entries"]
        for key in ("CMAKE_CXX_COMPILER", "CMAKE_C_COMPILER"):
            if entries.get(key):
                candidates.append(entries[key])
    if not candidates:
        fallback = shutil.which("c++")
        if fallback:
            candidates.append(fallback)
    compilers = []
    for compiler in dict.fromkeys(candidates):
        resolved = shutil.which(compiler) or compiler
        version = command_output([resolved, "--version"])
        compilers.append(
            {
                "path": resolved,
                "version": version.get("output"),
                "version_ok": version["ok"],
            }
        )
    return compilers


def host_provenance():
    return {
        "hostname": socket.gethostname(),
        "platform": platform.platform(),
        "system": platform.system(),
        "release": platform.release(),
        "machine": platform.machine(),
        "processor": platform.processor(),
        "python": {
            "version": platform.python_version(),
            "implementation": platform.python_implementation(),
            "executable": sys.executable,
            "full_version": sys.version,
        },
    }


def build_provenance(manifest_path, executables, executable_hashes, args):
    runner_path = pathlib.Path(__file__).resolve()
    git_record = git_provenance(runner_path.parent)
    repository_root = (
        pathlib.Path(git_record["root"])
        if git_record.get("available") else runner_path.parent.parent
    )
    source_candidates = [
        repository_root / "CMakeLists.txt",
        repository_root / "tools" / "run_ode_ceiling_sweep.py",
        repository_root / "tools" / "prepare_ode_ceiling_models.py",
        repository_root / "tools" / "gen_ode_ceiling_sweep.py",
    ]
    if "rk" in executables:
        source_candidates.append(
            repository_root / "tools" / "bench_ode_ceiling.cpp"
        )
    if "cvodes" in executables:
        source_candidates.append(
            repository_root / "tools" / "bench_ode_cvodes_ceiling.cpp"
        )
    source_hashes = [
        {"path": str(path), "sha256": sha256_file(path)}
        for path in source_candidates
        if path.is_file() and not path.is_symlink()
    ]
    cache_records = []
    by_cache = {}
    for kind, executable in executables.items():
        cache_path = find_cmake_cache(executable)
        if cache_path is None:
            continue
        key = str(cache_path)
        if key not in by_cache:
            by_cache[key] = {
                "path": key,
                "sha256": sha256_file(cache_path),
                "entries": relevant_cmake_cache(cache_path),
                "benchmarks": [],
            }
            cache_records.append(by_cache[key])
        by_cache[key]["benchmarks"].append(kind)
    return {
        "schema": "stanli.ode-ceiling-sweep.provenance.v1",
        "created_utc": datetime.datetime.now(
            datetime.timezone.utc
        ).isoformat(),
        "completed": False,
        "argv": sys.argv,
        "manifest": {
            "path": str(manifest_path),
            "sha256": sha256_file(manifest_path),
        },
        "runner": {
            "path": str(runner_path),
            "sha256": sha256_file(runner_path),
        },
        "git": git_record,
        "sources": source_hashes,
        "host": host_provenance(),
        "benchmarks": {
            kind: {
                "path": str(path),
                "sha256": executable_hashes[kind],
                "compile_flags": target_compile_flags(path),
            }
            for kind, path in executables.items()
        },
        "cmake_caches": cache_records,
        "compilers": compiler_provenance(cache_records),
        "selection": {
            "solvers": args.solvers,
            "case_names": args.case_names,
            "case_substrings": args.case_substrings,
            "point": args.point,
            "iterations": args.iterations,
            "batches": args.batches,
            "warmup_ms": args.warmup_ms,
            "timeout_seconds": args.timeout,
            "diagnostic": args.diagnostic,
        },
    }


def atomic_write_json(path, payload, exclusive=False):
    if exclusive:
        with path.open("x") as stream:
            json.dump(payload, stream, indent=2, sort_keys=True)
            stream.write("\n")
        return
    descriptor, temporary_name = tempfile.mkstemp(
        dir=path.parent, prefix=f".{path.name}.", suffix=".tmp"
    )
    temporary_path = pathlib.Path(temporary_name)
    try:
        with os.fdopen(descriptor, "w") as stream:
            json.dump(payload, stream, indent=2, sort_keys=True)
            stream.write("\n")
        os.replace(temporary_path, path)
    finally:
        if path_exists(temporary_path):
            temporary_path.unlink()


def _beta_continued_fraction(a, b, x):
    """Continued fraction used by the regularized incomplete beta."""
    maximum_iterations = 200
    epsilon = 3.0e-14
    minimum = 1.0e-300
    qab = a + b
    qap = a + 1.0
    qam = a - 1.0
    c = 1.0
    d = 1.0 - qab * x / qap
    if abs(d) < minimum:
        d = minimum
    d = 1.0 / d
    result = d
    for iteration in range(1, maximum_iterations + 1):
        even = 2 * iteration
        coefficient = (
            iteration * (b - iteration) * x
            / ((qam + even) * (a + even))
        )
        d = 1.0 + coefficient * d
        if abs(d) < minimum:
            d = minimum
        c = 1.0 + coefficient / c
        if abs(c) < minimum:
            c = minimum
        d = 1.0 / d
        result *= d * c
        coefficient = -(
            (a + iteration) * (qab + iteration) * x
            / ((a + even) * (qap + even))
        )
        d = 1.0 + coefficient * d
        if abs(d) < minimum:
            d = minimum
        c = 1.0 + coefficient / c
        if abs(c) < minimum:
            c = minimum
        d = 1.0 / d
        delta = d * c
        result *= delta
        if abs(delta - 1.0) <= epsilon:
            return result
    raise ArithmeticError("incomplete-beta continued fraction did not converge")


def regularized_incomplete_beta(a, b, x):
    if x <= 0.0:
        return 0.0
    if x >= 1.0:
        return 1.0
    front = math.exp(
        math.lgamma(a + b) - math.lgamma(a) - math.lgamma(b)
        + a * math.log(x) + b * math.log1p(-x)
    )
    if x < (a + 1.0) / (a + b + 2.0):
        return front * _beta_continued_fraction(a, b, x) / a
    return 1.0 - (
        front * _beta_continued_fraction(b, a, 1.0 - x) / b
    )


def student_t_cdf(value, degrees_freedom):
    if value == 0.0:
        return 0.5
    x = degrees_freedom / (degrees_freedom + value * value)
    tail = 0.5 * regularized_incomplete_beta(
        0.5 * degrees_freedom, 0.5, x
    )
    return 1.0 - tail if value > 0.0 else tail


def student_t_critical_95(degrees_freedom):
    low = 0.0
    high = 1.0
    while student_t_cdf(high, degrees_freedom) < 0.975:
        high *= 2.0
    for _ in range(80):
        middle = 0.5 * (low + high)
        if student_t_cdf(middle, degrees_freedom) < 0.975:
            low = middle
        else:
            high = middle
    return 0.5 * (low + high)


def paired_log_statistics(batches):
    samples = []
    for batch in batches:
        value = batch.get("speedup")
        if (
            isinstance(value, (int, float))
            and not isinstance(value, bool)
            and math.isfinite(value)
            and value > 0.0
        ):
            samples.append(float(value))
    logs = [math.log(value) for value in samples]
    result = {
        "method": "two-sided 95% Student-t interval on log(speedup)",
        "sample_count": len(samples),
        "geometric_mean": math.exp(sum(logs) / len(logs)) if logs else None,
        "confidence_level": 0.95,
        "degrees_freedom": len(logs) - 1 if len(logs) >= 2 else None,
        "ci_low": None,
        "ci_high": None,
    }
    if len(logs) < 2:
        return result
    mean = sum(logs) / len(logs)
    variance = sum((value - mean) ** 2 for value in logs) / (len(logs) - 1)
    standard_error = math.sqrt(variance / len(logs))
    critical = student_t_critical_95(len(logs) - 1)
    margin = critical * standard_error
    result.update(
        {
            "log_mean": mean,
            "log_sample_standard_deviation": math.sqrt(variance),
            "log_standard_error": standard_error,
            "t_critical": critical,
            "ci_low": math.exp(mean - margin),
            "ci_high": math.exp(mean + margin),
        }
    )
    return result


def case_name(row, manifest_index):
    return str(row.get("case", f"case_{manifest_index + 1}"))


def case_matches(name, exact_names, substrings):
    if not exact_names and not substrings:
        return True
    return name in exact_names or any(value in name for value in substrings)


def log_filename(case_index, name):
    slug = re.sub(r"[^A-Za-z0-9_.-]+", "_", name).strip("._") or "case"
    digest = hashlib.sha256(name.encode()).hexdigest()[:8]
    return f"{case_index:03d}_{slug[:80]}_{digest}.log"


def scalar(text):
    """Convert an unambiguous benchmark scalar, preserving opaque strings."""
    if text in ("true", "false"):
        return text == "true"
    try:
        return int(text, 0)
    except ValueError:
        pass
    try:
        value = float(text)
        return value if math.isfinite(value) else text
    except ValueError:
        return text


def key_values(line):
    return {
        match.group(1): scalar(match.group(2) or match.group(3))
        for match in KV_RE.finditer(line)
    }


def parse_rk_output(stdout):
    parsed = {
        "header": {},
        "shape": {},
        "provider": {},
        "correctness": {},
        "batches": [],
    }
    local = []
    compare_re = re.compile(
        r"^(?P<label>.+) bitwise=(?P<exact>\d+)/(?P<total>\d+) "
        r"max_relative=(?P<relative>\S+) worst=(?P<worst>\d+)$"
    )
    callbacks_re = re.compile(
        r"^callbacks oracle=(\d+) candidate=(\d+) equal=(\d+)$"
    )
    steps_re = re.compile(
        r"^steps candidate_accepted=(\d+) attempted_derived=(\d+) "
        r"rejected_derived=(\d+) initialization_rhs=(\d+)$"
    )
    batch_re = re.compile(
        r"^batch=(\d+) oracle_ns=(\S+) candidate_ns=(\S+) "
        r"speedup=(\S+)x$"
    )
    summary_re = re.compile(
        r"^median oracle_ns=(\S+) candidate_ns=(\S+) "
        r"paired_speedup=(\S+)x range=\[(\S+)x,(\S+)x\] "
        r"iterations=(\d+) batches=(\d+) sink=(\S+)$"
    )
    for line in stdout.splitlines():
        if line.startswith("model_lp="):
            parsed["header"] = key_values(line)
        elif line.startswith("shape "):
            parsed["shape"] = key_values(line)
        elif line.startswith("provider "):
            parsed["provider"] = key_values(line)
        elif line.startswith("timing_gate="):
            parsed["timing_gate"] = key_values(line)
        else:
            match = compare_re.match(line)
            if match:
                item = {
                    "label": match.group("label"),
                    "exact": int(match.group("exact")),
                    "total": int(match.group("total")),
                    "max_relative": scalar(match.group("relative")),
                    "worst": int(match.group("worst")),
                }
                if item["label"].startswith("local["):
                    local.append(item)
                else:
                    parsed["correctness"][item["label"]] = item
                continue
            match = callbacks_re.match(line)
            if match:
                parsed["callbacks"] = {
                    "oracle": int(match.group(1)),
                    "candidate": int(match.group(2)),
                    "equal": bool(int(match.group(3))),
                }
                continue
            match = steps_re.match(line)
            if match:
                parsed["steps"] = {
                    "accepted": int(match.group(1)),
                    "attempted_derived": int(match.group(2)),
                    "rejected_derived": int(match.group(3)),
                    "initialization_rhs": int(match.group(4)),
                }
                continue
            match = batch_re.match(line)
            if match:
                parsed["batches"].append(
                    {
                        "batch": int(match.group(1)),
                        "oracle_ns": scalar(match.group(2)),
                        "candidate_ns": scalar(match.group(3)),
                        "speedup": scalar(match.group(4)),
                    }
                )
                continue
            match = summary_re.match(line)
            if match:
                parsed["timing"] = {
                    "oracle_ns": scalar(match.group(1)),
                    "candidate_ns": scalar(match.group(2)),
                    "paired_speedup": scalar(match.group(3)),
                    "range_min": scalar(match.group(4)),
                    "range_max": scalar(match.group(5)),
                    "iterations": int(match.group(6)),
                    "batches": int(match.group(7)),
                }
    parsed["correctness"]["local"] = local
    return parsed


def parse_cvodes_output(stdout):
    parsed = {
        "header": {},
        "shape": {},
        "correctness": {},
        "profiles": {},
        "stats": {},
        "batches": [],
    }
    compare_re = re.compile(
        r"^(?P<label>solution (?:values|Jacobian)).*: "
        r"exact=(?P<exact>\d+)/(?P<total>\d+) "
        r"max_abs=(?P<absolute>\S+) max_rel=(?P<relative>\S+) "
        r"max_ulp=(?P<ulp>\d+) worst\[(?P<worst>\d+)\]="
        r"(?P<value>\S+) oracle=(?P<oracle>\S+)$"
    )
    batch_re = re.compile(
        r"^batch (\d+): oracle=(\S+) ns local=(\S+) ns speedup=(\S+)x$"
    )
    summary_re = re.compile(
        r"^uninstrumented medians: oracle=(\S+) ns local=(\S+) ns paired "
        r"speedup=(\S+)x \(range (\S+)x\.\.(\S+)x\) sink=(\S+)$"
    )
    for line in stdout.splitlines():
        if line.startswith("CVODES fvar ceiling/proximity experiment:"):
            parsed["header"] = key_values(line)
        elif line.startswith("shape "):
            parsed["shape"] = key_values(line)
        elif line.startswith("oracle profile ns:"):
            parsed["profiles"]["oracle"] = key_values(line)
        elif line.startswith("local profile ns:"):
            parsed["profiles"]["local"] = key_values(line)
        elif line.startswith("local callbacks:"):
            parsed["profiles"]["callbacks"] = key_values(line)
        elif line.startswith("CVODES stats:"):
            parsed["stats"]["solver"] = key_values(line)
        elif line.startswith("CVODES sensitivity stats:"):
            parsed["stats"]["sensitivity"] = key_values(line)
        elif line.startswith("timing_gate="):
            parsed["timing_gate"] = key_values(line)
        else:
            match = compare_re.match(line)
            if match:
                parsed["correctness"][match.group("label")] = {
                    "exact": int(match.group("exact")),
                    "total": int(match.group("total")),
                    "max_abs": scalar(match.group("absolute")),
                    "max_relative": scalar(match.group("relative")),
                    "max_ulp": int(match.group("ulp")),
                    "worst": int(match.group("worst")),
                    "candidate_value": scalar(match.group("value")),
                    "oracle_value": scalar(match.group("oracle")),
                }
                continue
            match = batch_re.match(line)
            if match:
                parsed["batches"].append(
                    {
                        "batch": int(match.group(1)),
                        "oracle_ns": scalar(match.group(2)),
                        "candidate_ns": scalar(match.group(3)),
                        "speedup": scalar(match.group(4)),
                    }
                )
                continue
            match = summary_re.match(line)
            if match:
                parsed["timing"] = {
                    "oracle_ns": scalar(match.group(1)),
                    "candidate_ns": scalar(match.group(2)),
                    "paired_speedup": scalar(match.group(3)),
                    "range_min": scalar(match.group(4)),
                    "range_max": scalar(match.group(5)),
                }
    return parsed


def validate_parsed_result(parsed, expected_batches, expected_iterations):
    """Return a reason when stdout is incomplete or numerically invalid."""
    timing_gate = parsed.get("timing_gate")
    if not isinstance(timing_gate, dict) or timing_gate.get("pass") != 1:
        return "missing or failed pre-timing correctness gate"

    batches = parsed.get("batches")
    if not isinstance(batches, list) or len(batches) != expected_batches:
        return (
            f"expected {expected_batches} complete batches, found "
            f"{len(batches) if isinstance(batches, list) else 0}"
        )
    if [batch.get("batch") for batch in batches] != list(
        range(1, expected_batches + 1)
    ):
        return "batch identifiers are missing, duplicated, or out of order"
    for batch in batches:
        for field in ("oracle_ns", "candidate_ns", "speedup"):
            value = batch.get(field)
            if (
                not isinstance(value, (int, float))
                or isinstance(value, bool)
                or not math.isfinite(value)
                or value <= 0.0
            ):
                return f"batch {batch.get('batch')} has invalid {field}"

    timing = parsed.get("timing")
    if not isinstance(timing, dict):
        return "timing summary not found"
    for field in (
        "oracle_ns", "candidate_ns", "paired_speedup", "range_min",
        "range_max",
    ):
        value = timing.get(field)
        if (
            not isinstance(value, (int, float))
            or isinstance(value, bool)
            or not math.isfinite(value)
            or value <= 0.0
        ):
            return f"timing summary has invalid {field}"
    if "batches" in timing and timing["batches"] != expected_batches:
        return "timing summary batch count does not match the request"
    if "iterations" in timing and timing["iterations"] != expected_iterations:
        return "timing summary iteration count does not match the request"
    return None


def selected_solvers(text):
    values = [value.strip().lower() for value in text.split(",") if value.strip()]
    if not values:
        raise argparse.ArgumentTypeError("select at least one solver")
    unknown = sorted(set(values) - set(SOLVERS))
    if unknown:
        raise argparse.ArgumentTypeError(
            "unknown solver(s): " + ", ".join(unknown)
        )
    return tuple(dict.fromkeys(values))


def arguments():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    parser.add_argument(
        "--rk-benchmark", type=pathlib.Path,
        default=pathlib.Path("build-rel/bench_ode_ceiling"),
    )
    parser.add_argument(
        "--cvodes-benchmark", type=pathlib.Path,
        default=pathlib.Path("build-rel/bench_ode_cvodes_ceiling"),
    )
    parser.add_argument(
        "--solvers", type=selected_solvers, default=SOLVERS,
        help="comma-separated subset of rk45,ckrk,bdf,adams",
    )
    parser.add_argument(
        "--case", "--case-name", dest="case_names", action="append",
        default=[], metavar="NAME",
        help="run an exact, case-sensitive manifest case name (repeatable)",
    )
    parser.add_argument(
        "--case-substr", dest="case_substrings", action="append",
        default=[], metavar="TEXT",
        help=(
            "run case names containing this case-sensitive text (repeatable; "
            "combined with --case by OR)"
        ),
    )
    parser.add_argument("--point", type=int, choices=(0, 1, 2), default=0)
    parser.add_argument("--iterations", type=int, default=1)
    parser.add_argument("--batches", type=int, default=3)
    parser.add_argument("--warmup-ms", type=int, default=0)
    parser.add_argument(
        "--diagnostic", action="store_true",
        help="ask RK benchmarks to emit perturbative component timings",
    )
    parser.add_argument("--timeout", type=float, default=120.0)
    parser.add_argument(
        "--force", action="store_true",
        help=(
            "replace regular output/sidecar files and a sentinel-owned log "
            "directory"
        ),
    )
    args = parser.parse_args()
    if (args.iterations < 1 or args.batches < 1 or args.warmup_ms < 0
            or args.timeout <= 0):
        parser.error(
            "iterations, batches, and timeout must be positive; "
            "warmup-ms must be nonnegative"
        )
    if any(not value for value in (*args.case_names, *args.case_substrings)):
        parser.error("case filters must not be empty")
    return args


def resolved_executable(path):
    path = path.expanduser().resolve()
    if not path.is_file():
        raise FileNotFoundError(path)
    return path


def main():
    args = arguments()
    manifest_path = args.manifest.expanduser().resolve()
    rows = json.loads(manifest_path.read_text())
    if not isinstance(rows, list):
        raise ValueError("manifest root must be a JSON array")
    cases = []
    hash_cache = {}
    for manifest_index, row in enumerate(rows):
        if not isinstance(row, dict):
            raise ValueError(f"manifest row {manifest_index} must be an object")
        solver = row.get("solver")
        name = case_name(row, manifest_index)
        if solver not in args.solvers or not case_matches(
            name, args.case_names, args.case_substrings
        ):
            continue
        if solver not in SOLVERS:
            raise ValueError(
                f"manifest row {manifest_index} has unknown solver {solver!r}"
            )
        try:
            mir_path = pathlib.Path(row["mir"]).expanduser().resolve()
            data_path = pathlib.Path(row["data"]).expanduser().resolve()
        except KeyError as error:
            raise ValueError(
                f"manifest row {manifest_index} is missing {error.args[0]!r}"
            ) from error
        if not mir_path.is_file():
            raise FileNotFoundError(mir_path)
        if not data_path.is_file():
            raise FileNotFoundError(data_path)
        cases.append(
            {
                "manifest_index": manifest_index,
                "row": row,
                "name": name,
                "solver": solver,
                "kind": "rk" if solver in RK_SOLVERS else "cvodes",
                "mir": mir_path,
                "data": data_path,
                "mir_sha256": sha256_file(mir_path, hash_cache),
                "data_sha256": sha256_file(data_path, hash_cache),
            }
        )
    if not cases:
        raise ValueError("no manifest cases matched the solver and case filters")

    benchmark_arguments = {
        "rk": args.rk_benchmark,
        "cvodes": args.cvodes_benchmark,
    }
    needed_kinds = {case["kind"] for case in cases}
    executables = {
        kind: resolved_executable(benchmark_arguments[kind])
        for kind in sorted(needed_kinds)
    }
    executable_hashes = {
        kind: sha256_file(path, hash_cache)
        for kind, path in executables.items()
    }

    # Capture repository state before output artifacts can affect dirty status.
    provenance = build_provenance(
        manifest_path, executables, executable_hashes, args
    )
    provenance["case_count"] = len(cases)

    output = absolute_path(args.output)
    log_dir = pathlib.Path(str(output) + ".logs")
    provenance_path = pathlib.Path(str(output) + PROVENANCE_SUFFIX)
    protected_paths = {
        manifest_path,
        pathlib.Path(__file__).resolve(),
        *executables.values(),
        *(case["mir"] for case in cases),
        *(case["data"] for case in cases),
    }
    for source in provenance.get("sources", []):
        protected_paths.add(pathlib.Path(source["path"]))
    for cache in provenance.get("cmake_caches", []):
        protected_paths.add(pathlib.Path(cache["path"]))
    for benchmark in provenance.get("benchmarks", {}).values():
        flags = benchmark.get("compile_flags")
        if flags is not None:
            protected_paths.add(pathlib.Path(flags["path"]))
    for row in rows:
        for key in ("stan", "stanc", "source_archive"):
            value = row.get(key) if isinstance(row, dict) else None
            if isinstance(value, str):
                protected_paths.add(pathlib.Path(value).expanduser().resolve())
        for key in ("mir", "data"):
            value = row.get(key) if isinstance(row, dict) else None
            if isinstance(value, str):
                protected_paths.add(pathlib.Path(value).expanduser().resolve())
    reject_output_collisions(
        output, log_dir, provenance_path, protected_paths
    )
    prepare_output_paths(
        output, log_dir, provenance_path, force=args.force
    )
    atomic_write_json(provenance_path, provenance, exclusive=True)

    started = time.monotonic()
    counts = {"ok": 0, "failed": 0, "timeout": 0, "parse_error": 0}
    with output.open("x") as stream:
        for index, case_record in enumerate(cases, 1):
            manifest_index = case_record["manifest_index"]
            row = case_record["row"]
            case = case_record["name"]
            solver = case_record["solver"]
            kind = case_record["kind"]
            command = [
                str(executables[kind]),
                str(case_record["mir"]),
                str(case_record["data"]),
                "--solver", str(solver),
                "--point", str(args.point),
                "--iterations", str(args.iterations),
                "--batches", str(args.batches),
                "--warmup-ms", str(args.warmup_ms),
            ]
            if row.get("require_exact") is True:
                command.append("--require-exact")
            if args.diagnostic and kind == "rk":
                command.append("--diagnostic")
            case_started = time.monotonic()
            stdout = ""
            stderr = ""
            returncode = None
            status = "failed"
            expected_hashes = {
                "mir": case_record["mir_sha256"],
                "data": case_record["data_sha256"],
                "selected_binary": executable_hashes[kind],
            }
            integrity_error = case_hash_error(
                case_record, executables[kind], expected_hashes
            )
            if integrity_error is not None:
                stderr = integrity_error
            else:
                try:
                    completed = subprocess.run(
                        command,
                        capture_output=True,
                        text=True,
                        timeout=args.timeout,
                        check=False,
                    )
                    stdout = completed.stdout
                    stderr = completed.stderr
                    returncode = completed.returncode
                    status = "ok" if returncode == 0 else "failed"
                except subprocess.TimeoutExpired as error:
                    stdout = error.stdout or ""
                    stderr = error.stderr or ""
                    if isinstance(stdout, bytes):
                        stdout = stdout.decode(errors="replace")
                    if isinstance(stderr, bytes):
                        stderr = stderr.decode(errors="replace")
                    status = "timeout"
                integrity_error = case_hash_error(
                    case_record, executables[kind], expected_hashes
                )
                if integrity_error is not None:
                    status = "failed"
                    stderr = (stderr.rstrip() + "\n" + integrity_error).lstrip()

            log_path = log_dir / log_filename(index, case)
            with log_path.open("x") as log_stream:
                log_stream.write(
                    "$ " + shlex.join(command) + "\n\n[stdout]\n" + stdout
                    + "\n[stderr]\n" + stderr
                )
            result = {
                "manifest_index": manifest_index,
                "case_index": index,
                "case_count": len(cases),
                "case": row,
                "status": status,
                "returncode": returncode,
                "elapsed_seconds": time.monotonic() - case_started,
                "command": command,
                "log": str(log_path),
                "stderr": stderr.strip(),
                "sha256": {
                    "mir": case_record["mir_sha256"],
                    "data": case_record["data_sha256"],
                    "selected_binary": executable_hashes[kind],
                },
                "mir_sha256": case_record["mir_sha256"],
                "data_sha256": case_record["data_sha256"],
                "selected_binary_sha256": executable_hashes[kind],
                "provenance": str(provenance_path),
            }
            if status == "ok":
                result["result"] = (
                    parse_rk_output(stdout)
                    if kind == "rk" else parse_cvodes_output(stdout)
                )
                validation_error = validate_parsed_result(
                    result["result"], args.batches, args.iterations
                )
                if validation_error is not None:
                    result["status"] = "parse_error"
                    result["parse_error"] = validation_error
                else:
                    paired_statistics = paired_log_statistics(
                        result["result"]["batches"]
                    )
                    result["result"]["paired_log_statistics"] = (
                        paired_statistics
                    )
                    if paired_statistics["sample_count"] != args.batches:
                        result["status"] = "parse_error"
                        result["parse_error"] = (
                            "paired statistics did not consume every batch"
                        )
            counts[result["status"]] += 1
            stream.write(json.dumps(result, sort_keys=True) + "\n")
            stream.flush()
            timing = result.get("result", {}).get("timing", {})
            ratio = timing.get("paired_speedup")
            ratio_text = f" {ratio:.3f}x" if isinstance(ratio, (int, float)) else ""
            print(
                f"[{index:02d}/{len(cases):02d}] {case}: "
                f"{result['status']}{ratio_text}",
                file=sys.stderr,
                flush=True,
            )

    summary = {
        "output": str(output),
        "logs": str(log_dir),
        "provenance": str(provenance_path),
        "cases": len(cases),
        "counts": counts,
        "elapsed_seconds": time.monotonic() - started,
        "iterations": args.iterations,
        "batches": args.batches,
        "warmup_ms": args.warmup_ms,
        "diagnostic": args.diagnostic,
        "point": args.point,
        "solvers": args.solvers,
        "case_names": args.case_names,
        "case_substrings": args.case_substrings,
    }
    provenance.update(
        {
            "completed": True,
            "completed_utc": datetime.datetime.now(
                datetime.timezone.utc
            ).isoformat(),
            "summary": summary,
        }
    )
    atomic_write_json(provenance_path, provenance)
    print(json.dumps(summary, indent=2))
    return 0 if counts["failed"] + counts["timeout"] + counts["parse_error"] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
