#!/usr/bin/env python3
"""Paired complete-gradient admission benchmark for the ODE runtime path.

Builds pinned CmdStan gradient drivers from both default and ``stanc --O1``
generated C++, then compares them with the same Release ``bench_grad`` binary
running the current ODE oracle and the direct RK candidate.  Every sample is a
fresh process with a 200 ms in-process warmup.  Batches alternate ABBA/BAAB
order and each measured arm is calibrated to the requested minimum duration.

Usage:
  tools/bench_ode_production.py deps/cmdstan --build build-rel
"""

import argparse
import hashlib
import json
import math
import os
import pathlib
import platform
import statistics
import subprocess
import sys
import tempfile
import zipfile


REPO = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "tools"))
from cmdstan_ref import compile_cmd  # noqa: E402


MODELS = (
    ("lotka_volterra", "hudson_lynx_hare"),
    ("soil_incubation", "soil_carbon"),
    ("one_comp_mm_elim_abs", "one_comp_mm_elim_abs"),
)

# Two-sided 95% Student-t critical values, indexed by degrees of freedom.
T95 = {
    1: 12.706, 2: 4.303, 3: 3.182, 4: 2.776, 5: 2.571,
    6: 2.447, 7: 2.365, 8: 2.306, 9: 2.262, 10: 2.228,
    11: 2.201, 12: 2.179, 13: 2.160, 14: 2.145, 15: 2.131,
    16: 2.120, 17: 2.110, 18: 2.101, 19: 2.093, 20: 2.086,
    21: 2.080, 22: 2.074, 23: 2.069, 24: 2.064, 25: 2.060,
    26: 2.056, 27: 2.052, 28: 2.048, 29: 2.045, 30: 2.042,
}


def run(command, *, env=None):
    result = subprocess.run(
        [str(part) for part in command], capture_output=True, text=True,
        cwd=REPO, env=env,
    )
    if result.returncode != 0:
        rendered = " ".join(str(part) for part in command)
        raise RuntimeError(
            f"command failed ({result.returncode}): {rendered}\n"
            f"{result.stderr[-4000:]}"
        )
    return result


def extract_data(archive, destination):
    with zipfile.ZipFile(archive) as source:
        members = [
            name for name in source.namelist()
            if name.endswith(".json") and not name.endswith("/")
        ]
        if len(members) != 1:
            raise RuntimeError(
                f"expected one JSON member in {archive}, found {len(members)}"
            )
        destination.write_bytes(source.read(members[0]))


def sha256_file(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def prepare_model(name, data_name, cmdstan, stanc, posteriordb, work):
    model_dir = work / name
    model_dir.mkdir()
    stan = posteriordb / "models/stan" / f"{name}.stan"
    archive = posteriordb / "data/data" / f"{data_name}.json.zip"
    data = model_dir / "data.json"
    mir = model_dir / "model.tmir.sexp"
    extract_data(archive, data)
    mir.write_text(run([stanc, "--O1", "--debug-optimized-mir", stan]).stdout)

    executables = {}
    for variant, stanc_flags in (("default", ()), ("o1", ("--O1",))):
        header = model_dir / f"{variant}.hpp"
        run([stanc, *stanc_flags, stan, f"--o={header}"])
        executable = model_dir / f"cmdstan_{variant}"
        run(compile_cmd(
            cmdstan, header, REPO / "tools/bench_cmdstan_grad.cpp",
            executable, opt="-O3",
        ))
        executables[variant] = executable
    return data, mir, executables


def reuse_model(name, work):
    model_dir = work / name
    data = model_dir / "data.json"
    mir = model_dir / "model.tmir.sexp"
    executables = {
        "default": model_dir / "cmdstan_default",
        "o1": model_dir / "cmdstan_o1",
    }
    for required in (data, mir, *executables.values()):
        if not required.is_file():
            raise FileNotFoundError(required)
    return data, mir, executables


def arm_environment(direct):
    env = os.environ.copy()
    if direct:
        env.pop("STANLI_NO_ODE_DIRECT_RK", None)
    else:
        env["STANLI_NO_ODE_DIRECT_RK"] = "1"
    env.pop("STANLI_ODE_DIRECT_RK", None)
    return env


def sample(command, count, env=None):
    fields = run([*command, str(count)], env=env).stdout.split()
    if not fields:
        raise RuntimeError(f"benchmark emitted no result: {command}")
    value = float(fields[0])
    if not math.isfinite(value) or value <= 0:
        raise RuntimeError(f"invalid benchmark time {value}: {command}")
    return value


def calibrate(command, env, minimum_seconds):
    probe_count = 100
    ns = sample(command, probe_count, env)
    count = max(1, math.ceil(minimum_seconds * 1e9 / ns))
    # Leave a little margin so normal timing noise cannot dip under the gate.
    return max(probe_count, math.ceil(count * 1.10))


def interval(ratios):
    logs = [math.log(value) for value in ratios]
    center = statistics.mean(logs)
    if len(logs) == 1:
        return math.exp(center), math.nan, math.nan
    error = statistics.stdev(logs) / math.sqrt(len(logs))
    critical = T95.get(len(logs) - 1, 1.96)
    return (
        math.exp(center),
        math.exp(center - critical * error),
        math.exp(center + critical * error),
    )


def paired_batches(arm_a, arm_b, counts, rounds):
    ratios = []
    raw = []
    for batch in range(rounds):
        order = ("a", "b", "b", "a") if batch % 2 == 0 else (
            "b", "a", "a", "b"
        )
        values = {"a": [], "b": []}
        for side in order:
            command, env = arm_a if side == "a" else arm_b
            values[side].append(sample(command, counts[side], env))
        a_ns = statistics.mean(values["a"])
        b_ns = statistics.mean(values["b"])
        ratio = a_ns / b_ns
        ratios.append(ratio)
        raw.append({"batch": batch + 1, "a_ns": a_ns, "b_ns": b_ns,
                    "ratio": ratio})
    return ratios, raw


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cmdstan", type=pathlib.Path)
    parser.add_argument("--build", type=pathlib.Path, default="build-rel")
    parser.add_argument("--posteriordb", type=pathlib.Path,
                        default="deps/posteriordb/posterior_database")
    parser.add_argument("--stanc", type=pathlib.Path,
                        default="deps/stanc3/stanc")
    parser.add_argument("--rounds", type=int, default=15)
    parser.add_argument("--min-seconds", type=float, default=0.5)
    parser.add_argument("--output", type=pathlib.Path)
    parser.add_argument("--keep", action="store_true")
    parser.add_argument(
        "--reuse-work", type=pathlib.Path,
        help="reuse a prior --keep directory instead of rebuilding CmdStan",
    )
    args = parser.parse_args()
    if args.rounds < 2 or not math.isfinite(args.min_seconds) or \
            args.min_seconds <= 0:
        parser.error("--rounds must be at least 2 and --min-seconds positive")

    output = args.output.resolve() if args.output else None
    output_tmp = (output.with_name(output.name + ".tmp") if output else None)
    for destination in (output, output_tmp):
        if destination and (destination.exists() or destination.is_symlink()):
            raise FileExistsError(destination)

    cmdstan = args.cmdstan.resolve()
    build = (REPO / args.build).resolve()
    posteriordb = (REPO / args.posteriordb).resolve()
    stanc = (REPO / args.stanc).resolve()
    bench_grad = build / "bench_grad"
    for required in (bench_grad, stanc):
        if not required.is_file():
            raise FileNotFoundError(required)

    work = (args.reuse_work.resolve() if args.reuse_work else
            pathlib.Path(tempfile.mkdtemp(prefix="stanli_ode_production_")))
    results = {
        "git_head": run(["git", "rev-parse", "HEAD"]).stdout.strip(),
        "cmdstan_head": run(
            ["git", "-C", cmdstan, "rev-parse", "HEAD"]
        ).stdout.strip(),
        "platform": platform.platform(),
        "bench_grad_sha256": sha256_file(bench_grad),
        "rounds": args.rounds,
        "minimum_seconds": args.min_seconds,
        "work": str(work),
        "complete": False,
        "models": [],
    }

    def save_results():
        if not output:
            return
        output_tmp.write_text(json.dumps(results, indent=2) + "\n")
        output_tmp.replace(output)
    comparisons = (
        ("oracle/direct", "oracle", "direct"),
        ("cmdstan_default/direct", "cmdstan_default", "direct"),
        ("cmdstan_o1/direct", "cmdstan_o1", "direct"),
    )

    for name, data_name in MODELS:
        print(f"preparing {name}", flush=True)
        if args.reuse_work:
            data, mir, executables = reuse_model(name, work)
        else:
            data, mir, executables = prepare_model(
                name, data_name, cmdstan, stanc, posteriordb, work
            )
        arms = {
            "oracle": ([bench_grad, mir, data], arm_environment(False)),
            "direct": ([bench_grad, mir, data], arm_environment(True)),
            "cmdstan_default": ([executables["default"], data], None),
            "cmdstan_o1": ([executables["o1"], data], None),
        }
        counts = {
            arm: calibrate(command, env, args.min_seconds)
            for arm, (command, env) in arms.items()
        }
        model_result = {
            "model": name,
            "counts": counts,
            "artifacts": {
                "mir_sha256": sha256_file(mir),
                "data_sha256": sha256_file(data),
                "cmdstan_default_sha256": sha256_file(executables["default"]),
                "cmdstan_o1_sha256": sha256_file(executables["o1"]),
            },
            "comparisons": [],
        }
        results["models"].append(model_result)
        for label, a_name, b_name in comparisons:
            ratios, raw = paired_batches(
                arms[a_name], arms[b_name],
                {"a": counts[a_name], "b": counts[b_name]}, args.rounds,
            )
            estimate, low, high = interval(ratios)
            model_result["comparisons"].append({
                "name": label, "estimate": estimate, "low": low,
                "high": high, "batches": raw,
            })
            print(
                f"{name} {label}: {estimate:.6f}x "
                f"[{low:.6f}, {high:.6f}]", flush=True
            )
            save_results()

    print("\n| model | comparison | geometric mean | paired 95% CI |")
    print("|---|---|---:|---:|")
    for model in results["models"]:
        for comparison in model["comparisons"]:
            print(
                f"| `{model['model']}` | {comparison['name']} | "
                f"{comparison['estimate']:.3f}x | "
                f"[{comparison['low']:.3f}x, {comparison['high']:.3f}x] |"
            )

    if output:
        results["complete"] = True
        save_results()
        print(f"results: {output}")
    if args.keep:
        print(f"artifacts: {work}")


if __name__ == "__main__":
    main()
