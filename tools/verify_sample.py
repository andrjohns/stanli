#!/usr/bin/env python3
"""Differential verification against CmdStan for a sample of passing models.

For each model: stanc emits C++, clang++ compiles it with tools/ref_driver.cpp
against the CmdStan tree (with -ffp-contract=off, matching CmdStan's own
build flags), both sides evaluate at the same deterministic unconstrained
point, and gradients are compared (relative 1e-10 pass threshold; measured
diffs are typically at or near bitwise).
lp must match exactly in policy terms: both sides use propto=false +
jacobian, so no propto constant is expected either.

Usage: tools/verify_sample.py CMDSTAN_DIR PDB_DIR model1 model2 ...
"""
import json
import pathlib
import subprocess
import sys
import tempfile
import zipfile

REPO = pathlib.Path(__file__).resolve().parent.parent


def main():
    cs = pathlib.Path(sys.argv[1])
    pdb = pathlib.Path(sys.argv[2]) / "posterior_database"
    models = sys.argv[3:]
    math = cs / "stan" / "lib" / "stan_math"
    lib = lambda pat: next((math / "lib").glob(pat))
    inc = [
        cs / "stan" / "src", math,
        next((cs / "stan" / "lib").glob("rapidjson_*")),
        lib("eigen_*"), lib("boost_*"),
        lib("sundials_*") / "include", lib("tbb_*") / "include",
    ]
    tmp = pathlib.Path(tempfile.mkdtemp(prefix="stanrt_verify_"))

    datas = {}
    for pj in sorted((pdb / "posteriors").glob("*.json")):
        meta = json.loads(pj.read_text())
        datas.setdefault(meta["model_name"], meta["data_name"])

    n_pass = 0
    for model in models:
        stan = pdb / "models" / "stan" / f"{model}.stan"
        dz = pdb / "data" / "data" / f"{datas[model]}.json.zip"
        dj = tmp / f"{model}_data.json"
        with zipfile.ZipFile(dz) as z:
            dj.write_bytes(z.read(z.namelist()[0]))

        hpp = tmp / f"{model}.hpp"
        subprocess.run([str(REPO / "deps/stanc3/stanc"), str(stan),
                        f"--o={hpp}"], check=True)
        exe = tmp / f"{model}_ref"
        tbb = math / "lib" / "tbb"
        # -ffp-contract=off matches CmdStan's own build (stan-math's
        # makefiles set it); without it the reference binary forms FMAs and
        # drifts a few ULP from what CmdStan actually computes.
        cmd = ["clang++", "-std=c++17", "-O1", "-ffp-contract=off",
               "-D_REENTRANT",
               "-DBOOST_DISABLE_ASSERTS", "-include", str(hpp),
               str(REPO / "tools/ref_driver.cpp"),
               f"-L{tbb}", "-ltbb", f"-Wl,-rpath,{tbb}",
               "-o", str(exe)]
        for i in inc:
            cmd.insert(3, f"-I{i}")
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode != 0:
            print(f"BUILD_FAIL {model}: {r.stderr.splitlines()[-1][:120]}")
            continue

        ref = subprocess.run([str(exe), str(dj)], capture_output=True,
                             text=True).stdout.split()
        got = subprocess.run([str(REPO / "build/stanrt_check"), str(stan),
                              str(dj)], capture_output=True, text=True,
                             cwd=REPO).stdout.split()
        if not ref or ref[0] != "OK" or not got or got[0] != "OK":
            print(f"RUN_FAIL {model}: ref={ref[:2]} got={got[:2]}")
            continue

        rv = [float(x) for x in ref[1:]]
        gv = [float(x) for x in got[1:]]
        if len(rv) != len(gv):
            print(f"SHAPE_FAIL {model}: {len(rv)} vs {len(gv)}")
            continue
        worst = 0.0
        for a, b in zip(rv, gv):
            scale = max(abs(a), abs(b), 1.0)
            worst = max(worst, abs(a - b) / scale)
        status = "VERIFIED" if worst < 1e-10 else "MISMATCH"
        if status == "VERIFIED":
            n_pass += 1
        print(f"{status} {model}: max rel diff {worst:.2e} over lp + "
              f"{len(rv) - 1} grads")

    print(f"\n{n_pass}/{len(models)} models verified against CmdStan")


if __name__ == "__main__":
    main()
