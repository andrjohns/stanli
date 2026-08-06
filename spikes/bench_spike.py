#!/usr/bin/env python3
"""Spike 1: re-roll ceiling. For each losing model, compare stanrt on the
original (unrolled scalar loop) vs the hand-vectorized variant, interleaved
A/B repeats, plus full-precision gradient parity. Run from worktree root."""
import pathlib
import statistics
import subprocess
import sys

SP = pathlib.Path("/private/tmp/claude-501/-Users-xitrium-claud/"
                  "6e0b81db-e3ff-4bff-a0cf-002fb9ad30f7/scratchpad/spike1")
REPO = pathlib.Path.cwd()
PDB = REPO / "deps/posteriordb/posterior_database/models/stan"
STANC = REPO / "deps/stanc3/stanc"
BENCH = REPO / "build-rel/bench_grad"
CHECK = REPO / "build-rel/stanrt_check"

MODELS = ["radon_pooled", "arK", "low_dim_gauss_mix"]
EVALS = {"radon_pooled": 30000, "arK": 50000, "low_dim_gauss_mix": 30000}
REPS = 3


def mir(stan_path, out):
    out.write_text(subprocess.run(
        [str(STANC), "--debug-transformed-mir", str(stan_path)],
        capture_output=True, text=True, check=True).stdout)


def grad_ns(sexp, data, n):
    out = subprocess.run([str(BENCH), str(sexp), str(data), str(n)],
                         capture_output=True, text=True, check=True)
    return float(out.stdout.split()[0])


def check_vals(stan_path, data):
    out = subprocess.run([str(CHECK), str(stan_path), str(data)],
                         capture_output=True, text=True, check=True).stdout
    toks = out.split()
    assert toks[0] == "OK", out[:200]
    return [float(t) for t in toks[1:]]


for m in MODELS:
    data = SP / f"{m}_data.json"
    orig_stan = PDB / f"{m}.stan"
    vec_stan = SP / f"{m}_vec.stan"

    o = check_vals(orig_stan, data)
    v = check_vals(vec_stan, data)
    assert len(o) == len(v)
    reldev = max(abs(a - b) / max(abs(a), 1e-300) for a, b in zip(o, v))

    so, sv = SP / f"{m}_orig.sexp", SP / f"{m}_vec.sexp"
    mir(orig_stan, so)
    mir(vec_stan, sv)

    n = EVALS[m]
    ons, vns = [], []
    for _ in range(REPS):
        ons.append(grad_ns(so, data, n))
        vns.append(grad_ns(sv, data, n))
    om, vm = statistics.median(ons), statistics.median(vns)
    print(f"{m}: orig={om:.0f}ns vec={vm:.0f}ns  "
          f"orig/vec={om / vm:.2f}x  max_reldev={reldev:.2e}")
    print(f"  orig reps: {[f'{x:.0f}' for x in ons]}")
    print(f"  vec  reps: {[f'{x:.0f}' for x in vns]}")
    sys.stdout.flush()
