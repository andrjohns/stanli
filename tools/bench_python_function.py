#!/usr/bin/env python3
"""Steady-state Function calls versus plain Python and vectorized NumPy.

Run with an installed package, or stage an optimized runtime in
python/stanli/_bin and use PYTHONPATH=python. Compilation is timed separately.
No JSON or source compilation occurs inside a timed function call.
"""
import argparse
import json
import platform
import statistics
import sys
import time
import timeit
from pathlib import Path

import numpy as np
import stanli


SOURCE = """
functions {
  real scalar_affine(real x, real a, real b) { return a * x + b; }
  vector vector_affine(vector x, real a, real b) { return a * x + b; }
}
model {}
"""


def affine(x, a, b):
    return a * x + b


def affine_loop(x, a, b):
    return [a * value + b for value in x]


def measure(implementations, samples, target_seconds):
    timers = {name: timeit.Timer(call) for name, call in implementations.items()}
    counts = {}
    raw = {name: [] for name in timers}
    for name, timer in timers.items():
        count = 1
        while True:
            elapsed = timer.timeit(count)
            if elapsed >= target_seconds or count >= 1_000_000:
                break
            count *= 2
        counts[name] = count
    names = list(timers)
    for sample in range(samples):
        # Rotate order each round to distribute thermal/scheduling drift.
        order = names[sample % len(names):] + names[:sample % len(names)]
        for name in order:
            raw[name].append(timers[name].timeit(counts[name]) / counts[name])
    return {
        name: {"median_us": statistics.median(values) * 1e6,
               "q25_us": float(np.quantile(values, .25)) * 1e6,
               "q75_us": float(np.quantile(values, .75)) * 1e6,
               "calls_per_sample": counts[name],
               "samples_us": [v * 1e6 for v in values]}
        for name, values in raw.items()
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--samples", type=int, default=9)
    parser.add_argument("--target-seconds", type=float, default=.03)
    parser.add_argument("--sizes", nargs="+", type=int, default=[1, 100, 10_000, 100_000])
    parser.add_argument("--output", type=Path, help="retain metadata and every raw sample")
    args = parser.parse_args()
    if args.samples < 3 or args.target_seconds <= 0 or min(args.sizes) < 0:
        parser.error("need at least 3 samples, positive target time, and nonnegative sizes")
    start = time.perf_counter()
    mir = stanli.stan_to_mir(SOURCE)
    compile_ms = (time.perf_counter() - start) * 1e3
    start = time.perf_counter()
    scalar = stanli.Function("scalar_affine", mir=mir)
    vector = stanli.Function("vector_affine", mir=mir)
    bind_ms = (time.perf_counter() - start) * 1e3
    report = {
        "python": sys.version, "numpy": np.__version__,
        "platform": platform.platform(), "machine": platform.machine(),
        "build_id": stanli.build_id(), "compile_ms": compile_ms,
        "two_handles_from_mir_ms": bind_ms, "samples": args.samples,
        "cases": {},
    }
    cases = [("scalar", {
        "stanli": lambda: scalar(x=1.25, a=2.5, b=-1.),
        "python": lambda: affine(x=1.25, a=2.5, b=-1.),
    })]
    for n in args.sizes:
        x = np.arange(n, dtype=np.float64) / 8
        xs = x.tolist()
        cases.append((f"vector[{n}]", {
            "stanli": lambda x=x: vector(x=x, a=2.5, b=-1.),
            "python": lambda xs=xs: affine_loop(x=xs, a=2.5, b=-1.),
            "numpy": lambda x=x: affine(x=x, a=2.5, b=-1.),
        }))
    print(f"Python {platform.python_version()}, NumPy {np.__version__}, {platform.machine()}")
    print(f"Runtime: {stanli.build_id()}")
    print(f"Compile: {compile_ms:.3f} ms; two handles from MIR: {bind_ms:.3f} ms")
    print("Times are microseconds/call: median [25th, 75th percentile].")
    for case, implementations in cases:
        expected = implementations["python"]()
        for call in implementations.values():
            np.testing.assert_allclose(call(), expected, rtol=0, atol=0)
        results = measure(implementations, args.samples, args.target_seconds)
        report["cases"][case] = results
        print(case)
        for name, result in results.items():
            print(f"  {name:6s} {result['median_us']:10.3f} "
                  f"[{result['q25_us']:.3f}, {result['q75_us']:.3f}]")
        print(f"  Stanli/Python: {results['stanli']['median_us'] / results['python']['median_us']:.2f}x")
        if "numpy" in results:
            print(f"  Stanli/NumPy:  {results['stanli']['median_us'] / results['numpy']['median_us']:.2f}x")
    if args.output:
        args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
