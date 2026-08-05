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
    print(f"\n== {len(ok)}/{len(results)} models pass ==")
    for m in ok:
        print(f"  OK {m}")
    print("\n== failure histogram ==")
    for k, c in reasons.most_common(30):
        print(f"  {c:3d}  {k}")

    md = ["# Corpus status", "", f"Passing: {len(ok)}/{len(results)}", ""]
    md += [f"- OK `{m}`" for m in ok]
    md += ["", "## Failures", ""]
    for model, (s, msg) in sorted(results.items()):
        if s != "OK":
            md.append(f"- `{model}`: {s} {msg}")
    (REPO / "docs" / "corpus-status.md").write_text("\n".join(md) + "\n")


if __name__ == "__main__":
    main()
