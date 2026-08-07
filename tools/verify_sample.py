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
import gzip
import json
import pathlib
import struct
import subprocess
import sys
import tempfile
import zipfile

REPO = pathlib.Path(__file__).resolve().parent.parent
REFS_PATH = REPO / "docs" / "corpus-refs.json.gz"


def write_results(results):
    """Merge into the machine-readable record the corpus scoreboard reads."""
    out = REPO / "docs" / "verification.json"
    prev = json.loads(out.read_text()) if out.exists() else {}
    prev.update(results)
    out.write_text(json.dumps(prev, indent=1, sort_keys=True) + "\n")


def write_refs(refs):
    """Merge the raw CmdStan values into the committed reference file.

    tools/verify_refs.py replays these without CmdStan installed, which is
    what lets CI run the differential corpus check on every push. Values are
    stored as the exact %.17g strings ref_driver printed, so they round-trip
    bitwise and diffs stay readable.
    """
    prev = {}
    if REFS_PATH.exists():
        prev = json.loads(gzip.decompress(REFS_PATH.read_bytes()))
    prev.update(refs)
    blob = json.dumps(prev, indent=0, sort_keys=True).encode()
    REFS_PATH.write_bytes(gzip.compress(blob, mtime=0))


def ulp_distance(a, b):
    """Distance in representable doubles; 0 means bitwise identical."""
    if a == b:
        return 0
    ia, ib = (struct.unpack("<q", struct.pack("<d", v))[0] for v in (a, b))
    key = lambda i: (-(1 << 63)) - i if i < 0 else i
    return abs(key(ia) - key(ib))


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
    tmp = pathlib.Path(tempfile.mkdtemp(prefix="stanli_verify_"))

    datas = {}
    for pj in sorted((pdb / "posteriors").glob("*.json")):
        meta = json.loads(pj.read_text())
        datas.setdefault(meta["model_name"], meta["data_name"])

    n_pass = 0
    results = {}
    refs = {}
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
        # ODE models pull in CVODES; CmdStan ships it prebuilt.
        sun = lib("sundials_*") / "lib"
        cmd = ["clang++", "-std=c++17", "-O1", "-ffp-contract=off",
               "-D_REENTRANT",
               "-DBOOST_DISABLE_ASSERTS", "-include", str(hpp),
               str(REPO / "tools/ref_driver.cpp"),
               f"-L{tbb}", "-ltbb", f"-Wl,-rpath,{tbb}",
               f"-L{sun}", "-lsundials_cvodes", "-lsundials_idas",
               "-lsundials_kinsol", "-lsundials_nvecserial",
               "-o", str(exe)]
        for i in inc:
            cmd.insert(3, f"-I{i}")
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode != 0:
            print(f"BUILD_FAIL {model}: {r.stderr.splitlines()[-1][:120]}")
            continue

        # Some models are invalid at a given point for BOTH engines (an ODE
        # solution dipping below a declared bound, say). Walk the shared
        # point list until one works on both sides.
        ref, got, point = [], [], 0
        for point in range(3):
            ref = subprocess.run([str(exe), str(dj), str(point)],
                                 capture_output=True, text=True).stdout.split()
            try:
                got = subprocess.run(
                    [str(REPO / "build/stanli_check"), str(stan), str(dj),
                     "--point", str(point)], capture_output=True, text=True,
                    cwd=REPO, timeout=300).stdout.split()
            except subprocess.TimeoutExpired:
                print(f"TIMEOUT {model}")
                got = []
                break
            if ref and ref[0] == "OK" and got and got[0] == "OK":
                break

        ref_ok = bool(ref) and ref[0] == "OK"
        got_ok = bool(got) and got[0] == "OK"
        if not ref_ok and not got_ok:
            # Both engines reject every shared point: the model is invalid
            # there (an ODE solution dipping below a declared bound, say),
            # so this is agreement, not a stanli failure. Recorded
            # separately and never counted as verified.
            results[model] = {"status": "REJECTED_BOTH", "max_rel": 0.0,
                              "max_ulp": 0, "n_values": 0, "point": point}
            write_results(results)
            print(f"REJECTED_BOTH {model}: CmdStan and stanli both reject "
                  f"every shared evaluation point")
            continue
        if not ref_ok or not got_ok:
            print(f"RUN_FAIL {model}: ref={ref[:2]} got={got[:2]}")
            continue

        rv = [float(x) for x in ref[1:]]
        gv = [float(x) for x in got[1:]]
        if len(rv) != len(gv):
            print(f"SHAPE_FAIL {model}: {len(rv)} vs {len(gv)}")
            continue

        worst = 0.0
        worst_ulp = 0
        for a, b in zip(rv, gv):
            scale = max(abs(a), abs(b), 1.0)
            worst = max(worst, abs(a - b) / scale)
            worst_ulp = max(worst_ulp, ulp_distance(a, b))
        status = "VERIFIED" if worst < 1e-10 else "MISMATCH"
        if status == "VERIFIED":
            n_pass += 1
        results[model] = {"status": status, "max_rel": worst,
                          "max_ulp": worst_ulp, "n_values": len(rv),
                          "point": point}
        write_results(results)  # incremental: a later hang keeps the rest
        # The raw CmdStan values, for tools/verify_refs.py to replay in CI
        # without CmdStan. `status` and `max_rel` ride along so a model
        # that is documented as not matching (kronecker_gp) is gated
        # against its recorded deviation rather than the clean threshold.
        refs[model] = {"point": point, "data": datas[model],
                       "values": ref[1:], "status": status,
                       "max_rel": worst}
        write_refs(refs)
        print(f"{status} {model}: max rel diff {worst:.2e} "
              f"({worst_ulp} ulp) over lp + {len(rv) - 1} grads")

    write_results(results)
    print(f"\n{n_pass}/{len(models)} models verified against CmdStan")


if __name__ == "__main__":
    main()
