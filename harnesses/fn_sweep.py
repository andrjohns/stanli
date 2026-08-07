#!/usr/bin/env python3
"""Per-function coverage and parity against CmdStan.

The corpus tells us the 120 models we care about are right. It says
nothing about a function no corpus model happens to use, and "supported"
in this project has twice meant "listed in a table no path could reach"
-- exponential_lpdf inside a parameter-dependent region was accepted by
the name lookup and refused by the block above it, silently, for a week.

This is the oracle for that. One tiny model per function, data chosen to
sit in the distribution's support, lowered by stanli and compiled by
CmdStan, both evaluated at the same points through the drivers the corpus
already uses (build-rel/stanli_check and tools/ref_driver.cpp). Reports
one line per function: supported or not, worst ULP against CmdStan, and
our per-gradient time so a new arrival that is accidentally quadratic is
visible immediately.

stanc3's own test corpus would be the obvious source of models, and is
not usable here: those files carry no data, and this lowering evaluates
transformed data eagerly and unrolls loops against known bounds, so a
model without data cannot be lowered at all. Generating the models is
what makes the data problem go away.

Usage:
  harnesses/fn_sweep.py deps/cmdstan [--filter SUBSTR] [--jobs N]
                                     [--keep] [--missing]

  --missing   also emit models for functions stan-math has and stanli
              does not, so the report doubles as the coverage gap.
"""
import argparse
import concurrent.futures
import json
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

REPO = pathlib.Path(__file__).resolve().parent.parent

# (stan name, arg count, argument values). The values matter: they have to
# sit inside the distribution's support, or both engines throw and the
# comparison says nothing. Outcome first, then parameters.
DENSITIES = [
    ("std_normal_lpdf", 1, [0.4]),
    ("normal_lpdf", 3, [0.4, 0.2, 1.3]),
    ("lognormal_lpdf", 3, [1.4, 0.2, 1.3]),
    ("exponential_lpdf", 2, [1.4, 0.7]),
    ("cauchy_lpdf", 3, [0.4, 0.2, 1.3]),
    ("gamma_lpdf", 3, [1.4, 2.0, 1.3]),
    ("inv_gamma_lpdf", 3, [1.4, 2.0, 1.3]),
    ("beta_lpdf", 3, [0.4, 2.0, 1.3]),
    ("weibull_lpdf", 3, [1.4, 2.0, 1.3]),
    ("logistic_lpdf", 3, [0.4, 0.2, 1.3]),
    ("double_exponential_lpdf", 3, [0.4, 0.2, 1.3]),
    ("uniform_lpdf", 3, [0.4, -1.0, 1.5]),
    ("student_t_lpdf", 4, [0.4, 3.0, 0.2, 1.3]),
    # Not wired into the log-density path yet; --missing includes them and
    # the report becomes the to-do list.
    ("chi_square_lpdf", 2, [1.4, 3.0]),
    ("inv_chi_square_lpdf", 2, [1.4, 3.0]),
    ("scaled_inv_chi_square_lpdf", 3, [1.4, 3.0, 1.2]),
    ("frechet_lpdf", 3, [1.4, 2.0, 1.3]),
    ("gumbel_lpdf", 3, [0.4, 0.2, 1.3]),
    ("loglogistic_lpdf", 3, [1.4, 2.0, 1.3]),
    ("pareto_lpdf", 3, [2.4, 1.0, 1.3]),
    ("pareto_type_2_lpdf", 4, [1.4, 0.0, 1.2, 1.3]),
    ("rayleigh_lpdf", 2, [1.4, 1.3]),
    ("skew_normal_lpdf", 4, [0.4, 0.2, 1.3, 0.7]),
    ("von_mises_lpdf", 3, [0.4, 0.2, 1.3]),
    ("exp_mod_normal_lpdf", 4, [0.4, 0.2, 1.3, 0.7]),
    ("beta_proportion_lpdf", 3, [0.4, 0.6, 2.0]),
    ("skew_double_exponential_lpdf", 4, [0.4, 0.2, 1.3, 0.6]),
]

# Which of the above stanli claims. Anything here that fails is a bug;
# anything absent that passes is free coverage nobody wired up.
def claimed(names_src):
    have = set(re.findall(r'"([a-z_0-9]+_(?:lpdf|lpmf))"', names_src))
    return have


MODEL = """data {{
  real y_data;
}}
parameters {{
  real p;
}}
model {{
  // Every argument is data except one, which carries the parameter, so
  // the gradient exercises the density's partial for that slot.
{body}
}}
"""


def model_for(name, argv):
    """A model whose target sums the density once per differentiable slot."""
    lines = []
    for k in range(len(argv)):
        args = [f"{v}" for v in argv]
        args[k] = f"({args[k]} + p * 0.0625)"
        lhs, rest = args[0], args[1:]
        call = f"{name}({lhs} | {', '.join(rest)})" if rest else f"{name}({lhs})"
        lines.append(f"  target += {call};")
    return MODEL.format(body="\n".join(lines))


def run(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def ulps(a, b):
    import struct
    if a == b:
        return 0
    ia = struct.unpack("<q", struct.pack("<d", a))[0]
    ib = struct.unpack("<q", struct.pack("<d", b))[0]
    if (ia < 0) != (ib < 0):
        return float("inf")
    return abs(ia - ib)


def sweep_one(spec, cs, tmp, keep):
    name, n, argv = spec
    d = tmp / name
    d.mkdir(parents=True, exist_ok=True)
    stan = d / f"{name}.stan"
    stan.write_text(model_for(name, argv[:n]))
    data = d / "data.json"
    data.write_text(json.dumps({"y_data": 1.0}))

    ours = run([str(REPO / "build-rel/stanli_check"), str(stan), str(data),
                "--stanc", str(REPO / "deps/stanc3/stanc")])
    if "COMPILE_FAIL" in ours.stdout or "COMPILE_FAIL" in ours.stderr:
        why = (ours.stdout + ours.stderr).strip().splitlines()[-1]
        return name, "unsupported", why[:60], None, None

    math = cs / "stan" / "lib" / "stan_math"
    hpp = d / f"{name}.hpp"
    if not run([str(REPO / "deps/stanc3/stanc"), str(stan), f"--o={hpp}"]).returncode == 0:
        return name, "stanc_fail", "", None, None
    exe = d / "ref"
    inc = [cs / "stan" / "src", math,
           next((cs / "stan" / "lib").glob("rapidjson_*")),
           next((math / "lib").glob("eigen_*")),
           next((math / "lib").glob("boost_*")),
           next((math / "lib").glob("sundials_*")) / "include",
           next((math / "lib").glob("tbb_*")) / "include"]
    tbb = math / "lib" / "tbb"
    build = run(["clang++", "-std=c++17", "-O1", "-ffp-contract=off",
                 "-D_REENTRANT", "-DBOOST_DISABLE_ASSERTS"]
                + [f"-I{i}" for i in inc]
                + ["-include", str(hpp), str(REPO / "tools/ref_driver.cpp"),
                   f"-L{tbb}", "-ltbb", f"-Wl,-rpath,{tbb}", "-o", str(exe)])
    if build.returncode != 0:
        return name, "ref_build_fail", build.stderr.strip()[-60:], None, None

    worst = 0
    for point in range(3):
        ref = run([str(exe), str(data), str(point)])
        got = run([str(REPO / "build-rel/stanli_check"), str(stan), str(data),
                   "--stanc", str(REPO / "deps/stanc3/stanc"),
                   "--point", str(point)])
        rf = [l for l in ref.stdout.splitlines() if l.startswith("OK")]
        gf = [l for l in got.stdout.splitlines() if l.startswith("OK")]
        if not rf or not gf:
            # Both rejecting the point is fine (outside the support at that
            # perturbation); one rejecting is not.
            if bool(rf) != bool(gf):
                return name, "one_side_threw", f"point {point}", None, None
            continue
        a = [float(x) for x in rf[0].split()[1:]]
        b = [float(x) for x in gf[0].split()[1:]]
        if len(a) != len(b):
            return name, "shape_mismatch", "", None, None
        worst = max([worst] + [ulps(x, y) for x, y in zip(a, b)])

    ns = None
    sexp = d / "m.sexp"
    mir = run([str(REPO / "deps/stanc3/stanc"), "--debug-transformed-mir", str(stan)])
    if mir.returncode == 0:
        sexp.write_text(mir.stdout)
        bench = run([str(REPO / "build-rel/bench_grad"), str(sexp), str(data), "20000"])
        if bench.returncode == 0 and bench.stdout.split():
            ns = float(bench.stdout.split()[0])
    if not keep:
        shutil.rmtree(d, ignore_errors=True)
    return name, "ok", "", worst, ns


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("cmdstan", type=pathlib.Path)
    ap.add_argument("--filter", default="")
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--missing", action="store_true")
    args = ap.parse_args()

    have = claimed((REPO / "runtime/src/lower.cpp").read_text())
    specs = [s for s in DENSITIES if args.filter in s[0]]
    if not args.missing:
        specs = [s for s in specs if s[0] in have]

    tmp = pathlib.Path(tempfile.mkdtemp(prefix="stanli_fnsweep_"))
    rows = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as ex:
        for r in ex.map(lambda s: sweep_one(s, args.cmdstan, tmp, args.keep),
                        specs):
            rows.append(r)
            print(".", end="", flush=True)
    print()

    print(f"{'function':<32} {'wired':<6} {'status':<16} {'ulp':>6} {'ns/grad':>10}")
    bad = 0
    for name, status, note, worst, ns in sorted(rows):
        wired = "yes" if name in have else "-"
        u = "" if worst is None else str(worst)
        t = "" if ns is None else f"{ns:,.0f}"
        print(f"{name:<32} {wired:<6} {status:<16} {u:>6} {t:>10}"
              + (f"  {note}" if note else ""))
        if name in have and status != "ok":
            bad += 1
        if status == "ok" and worst and worst > 2:
            bad += 1
    n_ok = sum(1 for r in rows if r[1] == "ok")
    print(f"\n{n_ok}/{len(rows)} evaluated and matched; "
          f"{sum(1 for r in rows if r[1] == 'unsupported')} unsupported")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
