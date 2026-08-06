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

# Verification results written by tools/verify_sample.py. A model counts as
# passing only if it appears here as VERIFIED; compiling and returning a
# finite gradient is never sufficient.
VERIFY_JSON = REPO / "docs" / "verification.json"


def load_verification():
    if not VERIFY_JSON.exists():
        return {}
    return json.loads(VERIFY_JSON.read_text())


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

    ver = load_verification()
    ok = sorted(m for m, (s, _) in results.items() if s == "OK")
    verified = [m for m in ok
                if ver.get(m, {}).get("status") == "VERIFIED"]
    print(f"\n== {len(ok)}/{len(results)} models evaluate "
          f"({len(verified)} verified vs CmdStan) ==")
    for m in ok:
        tag = "OK      " if m in verified else "EVAL-ONLY"
        print(f"  {tag} {m}")
    print("\n== failure histogram ==")
    for k, c in reasons.most_common(30):
        print(f"  {c:3d}  {k}")

    md = ["# Corpus status", "",
          f"Evaluating: {len(ok)}/{len(results)}",
          f"Differentially verified against CmdStan: "
          f"{len(verified)}/{len(results)}", "",
          "A model counts as passing only when tools/verify_sample.py "
          "matches CmdStan's log_prob and full gradient at the shared "
          "deterministic point. Accuracy below is the worst deviation "
          "over lp and every gradient component: relative, and in ULPs "
          "(0 = bitwise identical to CmdStan). Models that evaluate but "
          "are not verified are listed separately and are not counted.",
          "",
          "| model | values compared | max rel diff | max ULP |",
          "| --- | ---: | ---: | ---: |"]
    for m in verified:
        v = ver[m]
        rel = "0 (bitwise)" if v["max_rel"] == 0 else f"{v['max_rel']:.1e}"
        md.append(f"| `{m}` | {v['n_values']} | {rel} | {v['max_ulp']} |")
    unver = [m for m in ok if m not in verified]
    if unver:
        md += ["", "## Evaluate but not verified", ""]
        for m in unver:
            v = ver.get(m)
            why = (f"max rel diff {v['max_rel']:.1e}" if v
                   else "not yet run through verify_sample.py")
            md.append(f"- `{m}`: {why}")
    md += ["", "## Failures", ""]
    for model, (s, msg) in sorted(results.items()):
        if s != "OK":
            md.append(f"- `{model}`: {s} {msg}")
    (REPO / "docs" / "corpus-status.md").write_text("\n".join(md) + "\n")


if __name__ == "__main__":
    main()
