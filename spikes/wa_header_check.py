#!/usr/bin/env python3
"""Compare stanli's write_array CSV header against CmdStan's, model by model.

The column set and its order are a contract with every downstream reader
(posteriordb reference draws, ArviZ, bayesplot). Getting the values right is
not enough -- `array[K] simplex[K] theta` has to come out as theta.1.1 ..
theta.K.K, in that order, with the array index outermost.

CmdStan is the oracle: build the model, take one draw, drop the seven sampler
diagnostic columns, and diff.

Usage: python3 spikes/wa_header_check.py CMDSTAN_DIR PDB_DIR [MODEL ...]
With no MODEL arguments, checks every corpus model whose write_array lowers
completely -- which means a CmdStan build each, so it is not quick.
"""
import json
import pathlib
import subprocess
import sys
import tempfile
import zipfile

REPO = pathlib.Path(__file__).resolve().parent.parent
CHECK = REPO / "build-rel" / "stanli_check"
DIAG = 7  # lp__, accept_stat__, stepsize__, treedepth__, n_leapfrog__,
          # divergent__, energy__


def cmdstan_header(cmdstan, work, stan, data):
    # CmdStan's make derives the source path from the target path, so the
    # .stan has to sit next to where the executable will land.
    local = work / stan.name
    local.write_text(stan.read_text())
    exe = work / stan.stem
    r = subprocess.run(["make", str(exe)], cwd=cmdstan, capture_output=True,
                       text=True)
    if r.returncode != 0:
        return None, "build failed: " + r.stderr.strip().splitlines()[-1][:120]
    csv = work / f"{stan.stem}.csv"
    r = subprocess.run([str(exe), "sample", "num_warmup=2", "num_samples=1",
                        "data", f"file={data}", "output", f"file={csv}"],
                       cwd=work, capture_output=True, text=True)
    if not csv.exists():
        return None, "run failed: " + (r.stdout + r.stderr).strip()[-160:]
    for line in csv.read_text().splitlines():
        if line and not line.startswith("#"):
            return line.split(",")[DIAG:], ""
    return None, "no header in csv"


def main():
    cmdstan = pathlib.Path(sys.argv[1]).resolve()
    pdb = pathlib.Path(sys.argv[2]) / "posterior_database"
    want = set(sys.argv[3:])
    work = pathlib.Path(tempfile.mkdtemp(prefix="stanli_hdr_"))

    seen, ok, bad, skipped = set(), [], [], []
    for pj in sorted((pdb / "posteriors").glob("*.json")):
        meta = json.loads(pj.read_text())
        model = meta["model_name"]
        if model in seen or (want and model not in want):
            continue
        seen.add(model)
        stan = pdb / "models" / "stan" / f"{model}.stan"
        dz = pdb / "data" / "data" / f"{meta['data_name']}.json.zip"
        if not stan.exists() or not dz.exists():
            continue
        data = work / f"{meta['data_name']}.json"
        if not data.exists():
            with zipfile.ZipFile(dz) as z:
                data.write_bytes(z.read(z.namelist()[0]))

        p = subprocess.run([str(CHECK), str(stan), str(data), "--columns"],
                           capture_output=True, text=True, cwd=REPO)
        if p.returncode != 0:
            skipped.append((model, p.stdout.strip()[:110]))
            continue
        ours = p.stdout.strip().split(",")
        truncated = p.stderr.startswith("TRUNCATED")

        theirs, err = cmdstan_header(cmdstan, work, stan, data)
        if theirs is None:
            skipped.append((model, err))
            continue
        # A truncated write_array is legitimately a prefix, not a mismatch.
        ref = theirs[:len(ours)] if truncated else theirs
        if ours == ref:
            ok.append((model, len(ours), truncated))
        else:
            first = next((i for i in range(max(len(ours), len(ref)))
                          if i >= len(ours) or i >= len(ref)
                          or ours[i] != ref[i]), 0)
            bad.append((model, len(ours), len(ref), first,
                        ours[first:first + 4], ref[first:first + 4]))

    print(f"\n== {len(ok)} match, {len(bad)} differ, {len(skipped)} skipped ==")
    for m, n, tr in ok:
        print(f"  MATCH   {m:44s} {n:6d} cols{'  (prefix)' if tr else ''}")
    for m, no, nt, i, a, b in bad:
        print(f"  DIFFER  {m:44s} ours {no} cols, cmdstan {nt}; "
              f"first difference at {i}\n      ours    {a}\n      cmdstan {b}")
    for m, why in skipped:
        print(f"  skip    {m:44s} {why}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
