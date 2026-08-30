#!/usr/bin/env python3
"""Ablate Python Function packing and native lookup changes independently.

Preserve a baseline Release library before rebuilding the candidate, then run:
  PYTHONPATH=python python tools/bench_python_function_ab.py \
    --baseline-ref <commit> --baseline-library <saved-libstanli>

Both runtimes must support the typed-buffer Function ABI. Four isolated ctypes
adapters cross old/new Python code with old/new libraries in the same process.
Only the current library's compiler is used, outside the timed calls.
"""
import argparse
import ctypes
import hashlib
import json
import platform
import subprocess
import sys
import types
from pathlib import Path

import numpy as np
import stanli

from bench_python_function import SOURCE, measure


def adapter(source, library, name):
    # Each adapter needs its own CDLL wrapper: ctypes argtypes contain that
    # adapter's Structure/callback classes, even when the binary is shared.
    package = types.ModuleType(name)
    package._lib = ctypes.CDLL(str(library))
    package._read_utf8_file = stanli._read_utf8_file
    package.stan_to_mir = stanli.stan_to_mir
    sys.modules[name] = package
    module = types.ModuleType(name + "._function")
    module.__package__ = name
    exec(compile(source, name + "/_function.py", "exec"), module.__dict__)
    package._function = module
    return module.Function


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline-ref", required=True)
    parser.add_argument("--baseline-library", required=True, type=Path)
    parser.add_argument("--samples", type=int, default=11)
    parser.add_argument("--target-seconds", type=float, default=.05)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.samples < 3 or args.target_seconds <= 0:
        parser.error("need at least 3 samples and a positive target time")
    root = Path(__file__).resolve().parents[1]
    baseline_ref = subprocess.check_output(
        ["git", "rev-parse", args.baseline_ref], cwd=root, text=True).strip()
    old = subprocess.check_output(
        ["git", "show", baseline_ref + ":python/stanli/_function.py"], cwd=root)
    new = (root / "python/stanli/_function.py").read_bytes()
    baseline_library = args.baseline_library.resolve()
    current_library = Path(stanli._lib._name).resolve()
    variants = {
        "baseline": adapter(old, baseline_library, "_function_ab_old_old"),
        "packing_only": adapter(new, baseline_library, "_function_ab_new_old"),
        "lookup_only": adapter(old, current_library, "_function_ab_old_new"),
        "combined": adapter(new, current_library, "_function_ab_new_new"),
    }
    report = {
        "python": sys.version, "numpy": np.__version__,
        "platform": platform.platform(), "build_id": stanli.build_id(),
        "baseline_ref": baseline_ref, "samples": args.samples,
        "target_seconds": args.target_seconds,
        "baseline_library_sha256": hashlib.sha256(baseline_library.read_bytes()).hexdigest(),
        "current_library_sha256": hashlib.sha256(current_library.read_bytes()).hexdigest(),
        "current_adapter_sha256": hashlib.sha256(new).hexdigest(),
        "cases": {},
    }
    mir = stanli.stan_to_mir(SOURCE)
    scalar_args = {"x": 1.25, "a": 2.5, "b": -1.}
    cases = [("scalar", mir, "scalar_affine", scalar_args),
             ("numpy_scalars", mir, "scalar_affine",
              {k: np.float64(v) for k, v in scalar_args.items()})]
    for n in (0, 1, 100, 10_000, 100_000):
        cases.append((f"vector[{n}]", mir, "vector_affine",
                      {"x": np.arange(n, dtype=np.float64) / 8, "a": 2.5, "b": -1.}))
    cases.append(("strided_vector[100]", mir, "vector_affine",
                  {"x": np.arange(200.)[::-2], "a": 2.5, "b": -1.}))
    canary = stanli.stan_to_mir("""
functions {
  int increment(int count) { return count + 1; }
  real curve(real position) { return log1p_exp(position); }
}
model {}
""")
    cases.extend([("integer", canary, "increment", {"count": 41}),
                  ("nonlinear_canary", canary, "curve", {"position": -2.5})])
    unused = "\n".join(f"real unused_{i}(real y) {{ return y + {i}.0; }}"
                       for i in range(128))
    large_mir = stanli.stan_to_mir(SOURCE.replace("functions {", "functions {\n" + unused))
    cases.append(("scalar+128_definitions", large_mir, "scalar_affine", scalar_args))
    for label, program, name, arguments in cases:
        handles = {}
        for variant, constructor in variants.items():
            handles[variant] = constructor(name, mir=program)
        calls = {key: lambda f=f: f(**arguments) for key, f in handles.items()}
        expected = np.asarray(calls["baseline"]())
        for call in calls.values():
            actual = np.asarray(call())
            assert actual.dtype == expected.dtype and actual.shape == expected.shape
            assert actual.tobytes() == expected.tobytes(), label
        results = measure(calls, args.samples, args.target_seconds)
        report["cases"][label] = {"calls": results}
        print(label, flush=True)
        for variant, timing in results.items():
            print(f"  {variant:12s} {timing['median_us']:9.3f} "
                  f"[{timing['q25_us']:.3f}, {timing['q75_us']:.3f}] us", flush=True)
        if label in ("scalar", "scalar+128_definitions"):
            constructors = {key: lambda f=f: f(name, mir=program)
                            for key, f in variants.items()}
            # Parsing cached MIR and freeing a handle, not source compilation.
            report["cases"][label]["construct_and_free"] = measure(
                constructors, args.samples, args.target_seconds)
    if args.output:
        args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
