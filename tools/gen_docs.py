#!/usr/bin/env python3
"""Stamp the measured numbers into the docs, so they cannot go stale.

Every headline number in README.md and python/README.md (verified model
counts, bitwise counts, worst deviation, benchmark span, the PyPI page's
benchmark table) is derived from two artifacts:

  docs/verification.json   written by tools/verify_sample.py
  docs/benchmarks.md       per-gradient table, written when benchmarks run

The docs carry <!--gen:key-->...<!--/gen--> markers; this script replaces
the marked spans with values computed from the artifacts.

  tools/gen_docs.py           rewrite the docs in place
  tools/gen_docs.py --check   exit 1 if any doc disagrees with the
                              artifacts (CI runs this)

The prose around the markers is hand-written; only the numbers move.
"""
import json
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
TARGETS = [REPO / "README.md", REPO / "python" / "README.md"]
MARK = re.compile(r"(<!--gen:([a-z_]+)-->)(.*?)(<!--/gen-->)", re.S)


def bench_rows():
    """(model, params, stanli_ns, cmdstan_ns, speedup) from benchmarks.md."""
    text = (REPO / "docs" / "benchmarks.md").read_text()
    section = text.split("## Per-gradient latency")[1]
    rows = []
    # Only the first table in the section: stop at the first non-table
    # line after rows begin (the ODE section further down has its own
    # before/after table this must not swallow).
    for line in section.splitlines():
        if not line.startswith("|"):
            if rows:
                break
            continue
        if not line.startswith("| `"):
            continue
        cells = [c.strip() for c in line.strip("|").split("|")]
        rows.append((cells[0].strip("`"), int(cells[1]), float(cells[2]),
                     float(cells[3]), float(cells[4].rstrip("x"))))
    if not rows:
        raise SystemExit("no benchmark table found in docs/benchmarks.md")
    return rows


def us(ns):
    v = ns / 1000.0
    return f"{v:.2f} us" if v < 1 else f"{v:.1f} us"


def compute():
    ver = json.loads((REPO / "docs" / "verification.json").read_text())
    verified = {k: v for k, v in ver.items() if v["status"] == "VERIFIED"}
    bitwise = sum(1 for v in verified.values() if v["max_ulp"] == 0)
    worst = max(v["max_rel"] for v in verified.values())
    n_total = len(ver)

    rows = bench_rows()
    # A model counts as a win when its speedup rounds to at least 1.0x,
    # matching how the table has always been summarized.
    wins = [r for r in rows if round(r[4], 1) >= 1.0]
    losses = [r for r in rows if round(r[4], 1) < 1.0]
    span = (f"{min(round(r[4], 1) for r in wins):.1f}x-"
            f"{max(round(r[4], 1) for r in wins):.1f}x")
    loss_strs = [f"{r[4]:.2f}x" for r in sorted(losses, key=lambda r: -r[4])]
    loss_text = (" and ".join(loss_strs) if len(loss_strs) <= 2
                 else ", ".join(loss_strs[:-1]) + ", and " + loss_strs[-1])

    table = ["| model | params | stanli | CmdStan | speedup |",
             "| --- | ---: | ---: | ---: | ---: |"]
    for name, p, sns, cns, sp in rows:
        spd = f"**{sp:.1f}x**" if round(sp, 1) >= 1.0 else f"{sp:.2f}x"
        table.append(f"| `{name}` | {p} | {us(sns)} | {us(cns)} | {spd} |")

    return {
        "corpus_verified": f"{len(verified)}/{n_total}",
        "corpus_verified_of": f"{len(verified)} of {n_total}",
        "corpus_verified_n": str(len(verified)),
        "corpus_bitwise": str(bitwise),
        "corpus_worst": f"{worst:.1e}".replace("e-0", "e-"),
        "bench_span": span,
        "bench_wins": f"{len(wins)} of the {len(rows)}",
        "bench_losses": loss_text,
        "bench_table_us": "\n".join(table),
    }


def main():
    check = "--check" in sys.argv
    stats = compute()
    stale = []
    for path in TARGETS:
        text = path.read_text()
        rel = path.relative_to(REPO)

        def sub(m):
            key = m.group(2)
            if key not in stats:
                raise SystemExit(f"{rel}: unknown marker gen:{key}")
            if m.group(3) != stats[key]:
                short = (stats[key][:40] + "...") \
                    if len(stats[key]) > 40 else stats[key]
                stale.append(f"{rel}: {key}: {m.group(3)[:40]!r} -> {short!r}")
            return m.group(1) + stats[key] + m.group(4)

        new = MARK.sub(sub, text)
        if not check and new != text:
            path.write_text(new)
    if check and stale:
        print("docs disagree with the measured artifacts "
              "(run tools/gen_docs.py):")
        for s in stale:
            print(" ", s)
        return 1
    for s in stale:
        print("updated", s)
    if not stale:
        print("docs already match the artifacts")
    return 0


if __name__ == "__main__":
    sys.exit(main())
