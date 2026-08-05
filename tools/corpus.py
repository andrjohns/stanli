#!/usr/bin/env python3
"""Corpus coverage harness: run stanrt_check over every posteriordb
(model, dataset) pair and histogram the failures by missing feature.

Usage: tools/corpus.py PDB_DIR [--filter SUBSTR]
PDB_DIR is a posteriordb checkout containing posterior_database/.
Writes docs/corpus-status.md and prints the failure histogram.
"""
import collections
import json
import pathlib
import re
import subprocess
import sys
import tempfile
import zipfile

REPO = pathlib.Path(__file__).resolve().parent.parent
CHECK = REPO / "build" / "stanrt_check"

# Models that compile and produce a finite gradient but do NOT match
# CmdStan differentially. They are never counted as passing; entries leave
# only when tools/verify_sample.py agrees.
UNVERIFIED = {}


def main():
    pdb = pathlib.Path(sys.argv[1]) / "posterior_database"
    filt = sys.argv[sys.argv.index("--filter") + 1] if "--filter" in sys.argv else ""
    posteriors = sorted((pdb / "posteriors").glob("*.json"))
    tmp = pathlib.Path(tempfile.mkdtemp(prefix="stanrt_corpus_"))

    seen_models = set()
    results = {}
    reasons = collections.Counter()
    for pj in posteriors:
        meta = json.loads(pj.read_text())
        model = meta["model_name"]
        if model in seen_models or (filt and filt not in model):
            continue
        seen_models.add(model)
        stan = pdb / "models" / "stan" / f"{model}.stan"
        dz = pdb / "data" / "data" / f"{meta['data_name']}.json.zip"
        if not stan.exists() or not dz.exists():
            results[model] = ("SKIP", "missing files")
            continue
        dj = tmp / f"{meta['data_name']}.json"
        if not dj.exists():
            with zipfile.ZipFile(dz) as z:
                dj.write_bytes(z.read(z.namelist()[0]))
        try:
            out = subprocess.run(
                [str(CHECK), str(stan), str(dj)], capture_output=True,
                text=True, timeout=120, cwd=REPO).stdout.strip()
        except subprocess.TimeoutExpired:
            out = "EVAL_FAIL timeout"
        if out.startswith("OK"):
            results[model] = ("OK", "")
        else:
            first = out.split("\n")[0]
            status, _, msg = first.partition(" ")
            results[model] = (status, msg)
            # Classify by the interesting token.
            key = msg
            if "For loops" in msg:
                key = "For loops"
            elif "IfElse" in msg:
                key = "IfElse"
            elif "indexed assignment" in msg:
                key = "indexed assignment"
            else:
                m = re.search(r"unsupported (?:function |statement function )?([\w]+)", msg)
                if m:
                    key = f"unsupported {m.group(1)}"
            reasons[key] += 1

    ok = sorted(m for m, (s, _) in results.items() if s == "OK")
    verified = [m for m in ok if m not in UNVERIFIED]
    print(f"\n== {len(ok)}/{len(results)} models evaluate "
          f"({len(verified)} verified vs CmdStan) ==")
    for m in ok:
        print(f"  {'OK ' if m not in UNVERIFIED else 'EVAL-ONLY'} {m}")
    print("\n== failure histogram ==")
    for k, c in reasons.most_common(30):
        print(f"  {c:3d}  {k}")

    md = ["# Corpus status", "",
          f"Evaluating: {len(ok)}/{len(results)}",
          f"Differentially verified against CmdStan: "
          f"{len(verified)}/{len(results)}", "",
          "A model counts as verified only when tools/verify_sample.py "
          "matches CmdStan's log_prob and full gradient at the shared "
          "deterministic point. Models that compile and evaluate but do "
          "not yet match are listed as EVAL-ONLY and are not claimed as "
          "passing.", ""]
    md += [f"- OK `{m}`" for m in verified]
    if UNVERIFIED:
        md += ["", "## Evaluate but not verified", ""]
        md += [f"- `{m}`: {why}" for m, why in sorted(UNVERIFIED.items())
               if m in ok]
    md += ["", "## Failures", ""]
    for model, (s, msg) in sorted(results.items()):
        if s != "OK":
            md.append(f"- `{model}`: {s} {msg}")
    (REPO / "docs" / "corpus-status.md").write_text("\n".join(md) + "\n")


if __name__ == "__main__":
    main()
