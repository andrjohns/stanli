#!/usr/bin/env python3
"""What a brms fit needs beyond the code brms generates, against CmdStan.

The generated code itself is checked by tests/brms, which holds real
make_stancode output and is replayed against recorded CmdStan references
on every push. What is left here is the posterior predictive people write
around a fit for pp_check(): `yrep` drawn per observation and a `log_lik`
column, neither of which brms puts in the Stan file.

    harnesses/brms_sweep.py .                    # does it lower
    harnesses/brms_sweep.py . deps/cmdstan       # and is it right

Needs a CmdStan checkout for the second form, so it does not run in CI.
"""

import json
import pathlib
import tempfile
import subprocess
import sys

REPO = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
CMDSTAN = pathlib.Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else None
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent
                       / "tools"))
from cmdstan_ref import build_reference, compare_points  # noqa: E402
# A temp dir, NOT the harness directory: these models generate a .stan,
# a .hpp, and a compiled reference binary each, and writing them beside
# the source is how they end up in a commit.
HERE = pathlib.Path(tempfile.mkdtemp(prefix="brms_sweep_"))

CASES = {}

CASES["gq_predictions"] = ("""
data { int<lower=1> N; vector[N] Y; int prior_only; }
parameters { real Intercept; real<lower=0> sigma; }
transformed parameters {
  real lprior = 0;
  lprior += student_t_lpdf(Intercept | 3, 0, 2.5);
}
model {
  if (!prior_only) { target += normal_lpdf(Y | Intercept, sigma); }
  target += lprior;
}
generated quantities {
  real b_Intercept = Intercept;
  array[N] real yrep;
  vector[N] log_lik;
  for (n in 1:N) {
    yrep[n] = normal_rng(Intercept, sigma);
    log_lik[n] = normal_lpdf(Y[n] | Intercept, sigma);
  }
}
""", {"N": 8, "Y": [1.0, 2.0, 1.5, 0.5, 2.5, 1.1, 0.9, 1.8],
      "prior_only": 0})


MAX_REL = 1e-11
ABS_FLOOR = 1e-13


def reldiff(a, b):
    d = abs(a - b)
    if d == 0:
        return 0.0
    scale = max(abs(a), abs(b))
    if scale < ABS_FLOOR:
        return 0.0 if d < ABS_FLOOR else d
    return d / scale


def verify_one(cs, d, name, check, stanc):
    """lp and every gradient component, at three deterministic points."""
    stan, data = d / "m.stan", d / "d.json"
    exe, err = build_reference(cs, d, stan, REPO / "tools/ref_driver.cpp",
                               stanc, name=name, sundials=False)
    if exe is None:
        return err
    worst, n_cmp, err = compare_points(exe, check, stan, data, reldiff, stanc)
    if err:
        kind, detail = err
        return (f"one side threw at {detail}" if kind == "one_side_threw"
                else f"shape mismatch: {detail}")
    if n_cmp == 0:
        return "no valid point"
    if worst > MAX_REL:
        return f"{n_cmp} values, {worst:.2e} rel"
    return None if worst else None


def main():
    check = REPO / "build" / "stanli_check"
    stanc = REPO / "deps" / "stanc3" / "stanc"
    ok = 0
    for name, (src, data) in CASES.items():
        d = HERE / name
        d.mkdir(exist_ok=True)
        (d / "m.stan").write_text(src)
        (d / "d.json").write_text(json.dumps(data))
        r = subprocess.run(
            [str(check), str(d / "m.stan"), str(d / "d.json"),
             "--stanc", str(stanc)],
            capture_output=True, text=True)
        out = r.stdout + r.stderr
        if "COMPILE_FAIL" in out:
            why = [l for l in out.strip().splitlines()
                   if "COMPILE_FAIL" in l][0]
            print(f"FAIL  {name:22s} {why[:100]}")
        elif "EVAL_FAIL" in out:
            why = [l for l in out.strip().splitlines()
                   if "EVAL_FAIL" in l][0]
            print(f"EVAL  {name:22s} {why[:100]}")
        elif "OK" in out:
            if CMDSTAN is None:
                wa = [l for l in out.splitlines() if l.startswith("WA")]
                print(f"ok    {name:22s} lowers and evaluates"
                      f"{'  |  ' + wa[0] if wa else ''}")
                ok += 1
                continue
            # Lowering is not the bar. A model that compiles and returns
            # the wrong gradient is worse than one that refuses.
            bad = verify_one(CMDSTAN, d, name, check, stanc)
            if bad:
                print(f"FAIL  {name:22s} {bad}")
            else:
                print(f"ok    {name:22s} verified vs CmdStan")
                ok += 1
        else:
            print(f"?     {name:22s} {out.strip()[:100]}")
    what = "verified vs CmdStan" if CMDSTAN else "lower and evaluate"
    print(f"\n{ok}/{len(CASES)} brms-shaped models {what}")
    print(f"artifacts in {HERE}")


if __name__ == "__main__":
    main()
