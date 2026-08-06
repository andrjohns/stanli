#!/usr/bin/env python3
"""Corpus-wide head-to-head: stanli vs CmdStan on every posteriordb model.

Per model, both engines get one column each for
  - model preparation (stanli: lower+bind; CmdStan: stanc + full make)
  - per-gradient latency
  - end to end 1000 warmup + 1000 draws
Results stream to a TSV as they complete, so a partial run is still
useful and a rerun can skip what is already there.

Usage: python3 spikes/corpus_bench.py deps/cmdstan deps/posteriordb OUT.tsv
                                      [--filter SUBSTR] [--timeout SEC]
Run from the worktree root with build-rel/ built. Expect hours: CmdStan
builds a binary per model.
"""
import json
import os
import pathlib
import subprocess
import sys
import tempfile
import time
import zipfile

REPO = pathlib.Path.cwd()
BENCH = REPO / "build-rel/bench_grad"
RUN = REPO / "build-rel/stanli_run"
STANC = REPO / "deps/stanc3/stanc"
COLS = ["model", "params", "stanli_prep_s", "stanli_ns_grad",
        "stanli_sample_s", "cmdstan_build_s", "cmdstan_ns_grad",
        "cmdstan_sample_s", "note"]


def run(cmd, timeout, cwd=None):
    try:
        r = subprocess.run(cmd, capture_output=True, text=True,
                           timeout=timeout, cwd=cwd, env=dict(os.environ))
        return r if r.returncode == 0 else None
    except (subprocess.TimeoutExpired, OSError):
        return None


def evals_for(n):
    return 300 if n > 2000 else 3000 if n > 200 else 20000


def main():
    cs = pathlib.Path(sys.argv[1]).resolve()
    pdb = pathlib.Path(sys.argv[2]) / "posterior_database"
    out_path = pathlib.Path(sys.argv[3])
    filt = (sys.argv[sys.argv.index("--filter") + 1]
            if "--filter" in sys.argv else "")
    timeout = int(sys.argv[sys.argv.index("--timeout") + 1]
                  if "--timeout" in sys.argv else 900)
    tmp = pathlib.Path(tempfile.mkdtemp(prefix="stanli_cb_"))

    done = set()
    if out_path.exists():
        for line in out_path.read_text().splitlines()[1:]:
            done.add(line.split("\t")[0])
    else:
        out_path.write_text("\t".join(COLS) + "\n")

    pairs = {}
    for pj in sorted((pdb / "posteriors").glob("*.json")):
        meta = json.loads(pj.read_text())
        pairs.setdefault(meta["model_name"], meta["data_name"])

    for model, dname in sorted(pairs.items()):
        if (filt and filt not in model) or model in done:
            continue
        stan = pdb / "models" / "stan" / f"{model}.stan"
        dz = pdb / "data" / "data" / f"{dname}.json.zip"
        if not stan.exists() or not dz.exists():
            continue
        dj = tmp / f"{model}.json"
        with zipfile.ZipFile(dz) as z:
            dj.write_bytes(z.read(z.namelist()[0]))
        row = {c: "" for c in COLS}
        row["model"] = model
        notes = []

        # ---- stanli ----
        sexp = tmp / f"{model}.sexp"
        r = run([str(STANC), "--debug-transformed-mir", str(stan)], timeout)
        if r is None:
            notes.append("stanc_fail")
        else:
            sexp.write_text(r.stdout)
            probe = run([str(BENCH), str(sexp), str(dj), "1"], timeout)
            if probe is None:
                notes.append("stanli_eval_fail")
            else:
                n_params = int(probe.stdout.split()[-1])
                row["params"] = n_params
                t0 = time.perf_counter()
                run([str(BENCH), str(sexp), str(dj), "1"], timeout)
                row["stanli_prep_s"] = f"{time.perf_counter() - t0:.3f}"
                g = run([str(BENCH), str(sexp), str(dj),
                         str(evals_for(n_params))], timeout)
                if g:
                    row["stanli_ns_grad"] = f"{float(g.stdout.split()[0]):.0f}"
                t0 = time.perf_counter()
                s = run([str(RUN), str(stan), str(dj), "--warmup", "1000",
                         "--samples", "1000", "--seed", "1"], timeout)
                if s:
                    row["stanli_sample_s"] = f"{time.perf_counter() - t0:.2f}"
                else:
                    notes.append("stanli_sample_timeout")

        # ---- CmdStan: real model binary, built the way users build it ----
        work = tmp / model
        work.mkdir(exist_ok=True)
        (work / f"{model}.stan").write_text(stan.read_text())
        exe = work / model
        t0 = time.perf_counter()
        b = run(["make", str(exe)], timeout, cwd=str(cs))
        row["cmdstan_build_s"] = f"{time.perf_counter() - t0:.1f}"
        if b is None:
            notes.append("cmdstan_build_fail")
        else:
            hpp = work / f"{model}.hpp"
            if hpp.exists():
                math = cs / "stan" / "lib" / "stan_math"
                inc = [cs / "stan" / "src", math,
                       next((cs / "stan" / "lib").glob("rapidjson_*")),
                       next((math / "lib").glob("eigen_*")),
                       next((math / "lib").glob("boost_*")),
                       next((math / "lib").glob("sundials_*")) / "include",
                       next((math / "lib").glob("tbb_*")) / "include"]
                tbb = math / "lib" / "tbb"
                gexe = work / "gradbench"
                cmd = (["clang++", "-std=c++17", "-O3", "-ffp-contract=off",
                        "-D_REENTRANT", "-DBOOST_DISABLE_ASSERTS"] +
                       [f"-I{i}" for i in inc] +
                       ["-include", str(hpp),
                        str(REPO / "tools/bench_cmdstan_grad.cpp"),
                        f"-L{tbb}", "-ltbb", f"-Wl,-rpath,{tbb}",
                        "-o", str(gexe)])
                if run(cmd, timeout):
                    n_params = int(row["params"] or 0)
                    g = run([str(gexe), str(dj), str(evals_for(n_params))],
                            timeout)
                    if g:
                        row["cmdstan_ns_grad"] = f"{float(g.stdout.split()[0]):.0f}"
            t0 = time.perf_counter()
            s = run([str(exe), "sample", "num_warmup=1000",
                     "num_samples=1000", "random", "seed=1",
                     "data", f"file={dj}",
                     "output", f"file={work}/out.csv"], timeout)
            if s:
                row["cmdstan_sample_s"] = f"{time.perf_counter() - t0:.2f}"
            else:
                notes.append("cmdstan_sample_timeout")

        row["note"] = ",".join(notes)
        with out_path.open("a") as f:
            f.write("\t".join(str(row[c]) for c in COLS) + "\n")
        print(f"{model}: stanli {row['stanli_ns_grad']}ns/"
              f"{row['stanli_sample_s']}s  cmdstan {row['cmdstan_ns_grad']}ns/"
              f"{row['cmdstan_sample_s']}s  {row['note']}", flush=True)


if __name__ == "__main__":
    main()
