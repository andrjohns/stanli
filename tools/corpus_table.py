#!/usr/bin/env python3
"""Render docs/corpus-bench.tsv as the markdown table docs/benchmarks.md
embeds: every model, sorted by per-gradient speedup, with sampling times
where both engines completed.

Usage: python3 tools/corpus_table.py docs/corpus-bench.tsv
Prints markdown to stdout; benchmarks.md is edited by hand around it.
"""
import sys


def fmt_ns(v):
    return f"{int(float(v)):,}" if v else "-"


def fmt_ratio(a, b):
    if not a or not b:
        return "-"
    return f"{float(a) / float(b):.2f}x"


def main():
    rows = []
    with open(sys.argv[1]) as f:
        header = f.readline().rstrip("\n").split("\t")
        idx = {name: k for k, name in enumerate(header)}
        for line in f:
            c = line.rstrip("\n").split("\t")
            if len(c) < len(header):
                c += [""] * (len(header) - len(c))
            rows.append(c)

    def col(r, name):
        return r[idx[name]].strip()

    def grad_ratio(r):
        a, b = col(r, "stanli_ns_grad"), col(r, "cmdstan_ns_grad")
        if not a or not b:
            return -1.0
        return float(b) / float(a)

    rows.sort(key=grad_ratio, reverse=True)

    print("| model | params | stanli ns/grad | CmdStan ns/grad | grad speedup |"
          " stanli sample | CmdStan sample | sample speedup | note |")
    print("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |")
    for r in rows:
        model = col(r, "model")
        note = col(r, "note")
        ss, cs = col(r, "stanli_sample_s"), col(r, "cmdstan_sample_s")
        print(f"| `{model}` | {col(r, 'params')} "
              f"| {fmt_ns(col(r, 'stanli_ns_grad'))} "
              f"| {fmt_ns(col(r, 'cmdstan_ns_grad'))} "
              f"| {fmt_ratio(col(r, 'cmdstan_ns_grad'), col(r, 'stanli_ns_grad'))} "
              f"| {ss + ' s' if ss else '-'} "
              f"| {cs + ' s' if cs else '-'} "
              f"| {fmt_ratio(cs, ss)} "
              f"| {note} |")

    n = len(rows)
    ratios = sorted(g for g in (grad_ratio(r) for r in rows) if g > 0)
    at_par = sum(1 for g in ratios if g >= 1.0)
    med = ratios[len(ratios) // 2] if ratios else 0
    print()
    print(f"{n} models; median per-gradient speedup {med:.2f}x; "
          f"{at_par}/{len(ratios)} at or above CmdStan.")


if __name__ == "__main__":
    main()
