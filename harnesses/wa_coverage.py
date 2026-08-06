#!/usr/bin/env python3
"""How much of the corpus gets its write_array -- transformed parameters and
generated quantities -- and what stops the rest.

stanli_check evaluates the write_array graph at the same deterministic point
it uses for the gradient and reports on stderr:

    WA <nvars> vars <nvalues> values <nbad> nonfinite complete|truncated: ...
    WA empty <why>          the graph could not be driven by the draw
    WA none                 the MIR had no generate_quantities section

Usage: python3 harnesses/wa_coverage.py PDB_DIR [--filter SUBSTR]
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
CHECK = REPO / "build-rel" / "stanli_check"


def main():
    pdb = pathlib.Path(sys.argv[1]) / "posterior_database"
    filt = (sys.argv[sys.argv.index("--filter") + 1]
            if "--filter" in sys.argv else "")
    tmp = pathlib.Path(tempfile.mkdtemp(prefix="stanli_wa_"))

    seen, rows, reasons = set(), [], collections.Counter()
    for pj in sorted((pdb / "posteriors").glob("*.json")):
        meta = json.loads(pj.read_text())
        model = meta["model_name"]
        if model in seen or (filt and filt not in model):
            continue
        seen.add(model)
        stan = pdb / "models" / "stan" / f"{model}.stan"
        dz = pdb / "data" / "data" / f"{meta['data_name']}.json.zip"
        if not stan.exists() or not dz.exists():
            continue
        dj = tmp / f"{meta['data_name']}.json"
        if not dj.exists():
            with zipfile.ZipFile(dz) as z:
                dj.write_bytes(z.read(z.namelist()[0]))
        try:
            p = subprocess.run([str(CHECK), str(stan), str(dj)],
                               capture_output=True, text=True, timeout=300,
                               cwd=REPO)
        except subprocess.TimeoutExpired:
            rows.append((model, "TIMEOUT", "", 0))
            continue
        if not p.stdout.startswith("OK"):
            continue  # the model does not compile at all; corpus.py covers it
        wa = ""
        for line in p.stderr.splitlines():
            if line.startswith("WA "):
                wa = line[3:]
        if wa.startswith("none"):
            rows.append((model, "NO_GQ_SECTION", "", 0))
            continue
        if wa.startswith("empty"):
            rows.append((model, "EMPTY", wa[6:], 0))
            reasons[short(wa[6:])] += 1
            continue
        m = re.match(r"(\d+) vars (\d+) values (\d+) nonfinite (.*)", wa)
        if not m:
            rows.append((model, "UNPARSED", wa, 0))
            continue
        nvars, nvals, nbad, tail = (int(m[1]), int(m[2]), int(m[3]), m[4])
        # Nonfinite values are a note, not a verdict: a model can legitimately
        # write -inf (Survey_model's lp_parts is -inf below its support), and
        # conflating that with a lowering failure hid the truncation reason.
        note = f", {nbad}/{nvals} nonfinite" if nbad else ""
        if tail == "complete":
            rows.append((model, "COMPLETE", f"{nvars} vars{note}", nvals))
        else:
            rows.append((model, "TRUNCATED", tail + note, nvals))
            reasons[short(tail)] += 1

    by = collections.Counter(r[1] for r in rows)
    print(f"{len(rows)} compiling models")
    for k in ("COMPLETE", "TRUNCATED", "NONFINITE", "EMPTY", "NO_GQ_SECTION",
              "TIMEOUT", "UNPARSED"):
        if by[k]:
            print(f"  {by[k]:4d}  {k}")
    if reasons:
        print("\nwhat stops the rest:")
        for k, c in reasons.most_common(30):
            print(f"  {c:4d}  {k}")
    print("\nper model:")
    for model, status, detail, _ in sorted(rows, key=lambda r: (r[1], r[0])):
        print(f"  {status:14s} {model:44s} {detail[:110]}")


def short(msg):
    m = re.search(r"unsupported (?:function |statement function )?([\w]+)",
                  msg)
    if m:
        return f"unsupported {m.group(1)}"
    return msg.split("|")[0].strip()[:80]


if __name__ == "__main__":
    main()
