# Benchmarks vs CmdStan

2026-08-06, Apple M-series (macOS arm64), clang, both sides -O3 and
-ffp-contract=off, single-threaded. Both engines evaluate the sampling
gradient (propto + jacobian) at the same deterministic unconstrained
point. stanrt runs `tools/bench_grad.cpp`; CmdStan runs
`tools/bench_cmdstan_grad.cpp` compiled against the stanc-generated
header, looping the same fresh-vars + grad + recover_memory cycle
`stan::model::gradient` performs per leapfrog step. Reproduce the whole
table with `tools/bench_models.py deps/cmdstan deps/posteriordb`.

## Per-gradient latency

| model | unconstrained params | stanrt ns/grad | CmdStan ns/grad | speedup | stanrt prep | CmdStan build |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `radon_pooled` | 3 | 52730 | 325859 | 6.18x | 0.079 s | 6.2 s |
| `arK` | 7 | 2440 | 11994 | 4.91x | 0.007 s | 6.4 s |
| `bym2_offset_only` | 3845 | 40126 | 121974 | 3.04x | 0.046 s | 7.6 s |
| `kidscore_momiq` | 3 | 1882 | 3921 | 2.08x | 0.006 s | 6.4 s |
| `eight_schools_noncentered` | 10 | 279 | 563 | 2.02x | 0.004 s | 6.8 s |
| `lsat_model` | 1006 | 46233 | 91582 | 1.98x | 0.053 s | 7.1 s |
| `wells_dist100ars_model` | 3 | 17311 | 18263 | 1.06x | 0.025 s | 6.7 s |
| `low_dim_gauss_mix` | 5 | 184510 | 98153 | 0.53x | 0.194 s | 6.7 s |

**Where the wins come from: op granularity.** The interpreter's cost is
per op, not per element: ~17-20 ns for a scalar density op forward +
backward, measured as ~9.5 ns executor (dispatch + context assembly) plus
~9 ns recorder/sink inside the kernel, against ~0.9 ns for the actual
math (`tools/bench_opcost.cpp`). A vectorized statement over N elements
amortizes that to nothing and runs precompiled stan-math on contiguous
doubles, while CmdStan pays its var-tape cost per scalar per evaluation:
one tape node allocated, walked, and freed per leapfrog step, with
AoS-strided access. Vectorized models therefore win (2-6x, growing with
N), and models that were stuck as unrolled scalar loops used to lose
(0.4-0.9x before the re-roll pass).

**The re-roll pass** (`runtime/src/reroll.cpp`) closes that gap at the
graph level. Lowering unrolls data-bound loops, so a scalar-loop model
arrives as N consecutive copies of a small op template; the pass detects
these periodic regions and rewrites them into the vectorized ops the
kernels already support (constant vectors materialized from the const
pool, invariant ops hoisted, elementwise lanes widened, INDEX
progressions collapsed into their base vector, and N scalar density
terms fused into one summed vector density). `radon_pooled` collapses
from 27,670 ops to 8 (0.91x -> 6.18x), `arK` from 3,164 to 21 (0.40x ->
4.91x), `rats_model` from 939 to 30 - block-structured data re-rolls per
block. Anything the pass cannot prove safe it leaves alone, per region:
cross-lane recurrences, partial/strided index progressions, outputs
escaping their lane, opcodes outside its vocabulary. Set
`STANRT_NO_REROLL=1` to disable it.

**The remaining loser, `low_dim_gauss_mix` (0.53x),** is the documented
phase-2 case: its per-observation `log_mix(theta, normal_lpdf, ...)`
means the density outputs feed an op instead of the target, so the pass
correctly bails. It needs an elementwise-lp density variant plus a
batched `log_sum_exp`/`log_mix` kernel; the hand-vectorized spike puts
the available win at 2x+ (parity with CmdStan just from vectorizing the
two normal chains, more with the batched kernel).

Model preparation scales too: the largest model in the corpus
(`nn_rbm1bJ100`, MNIST, 60,000 rows, 79,411 parameters) lowers to a
192,030-op graph in 20.7 s and evaluates its gradient in 0.43 s. That
number used to be unbounded: the transformed-data interpreter evaluated
an indexed expression by copying its base, so reading `y[n]` in a loop
copied the whole array each time and lowering was quadratic in the data
size.

Preparation time is the other axis: stanrt lowers a model in 4-200 ms,
against a 6.2-7.6 s CmdStan compile (with a warm precompiled header, and
after a multi-minute one-time `make build`). That gap is what
time-to-first-draw is made of. Re-rolling also cut preparation time on
loop-heavy models (radon_pooled 0.39 s -> 0.08 s): the executor binds
and sizes 8 ops instead of 27,670.

## End to end: eight schools, model.stan + data.json -> 1000 warmup + 1000 draws

| engine | stage | time |
| --- | --- | --- |
| stanrt | stanc + graph compile + bind | 0.014 s |
| stanrt | NUTS 1000+1000 (incl. constraining draws) | 0.020 s |
| stanrt | CLI total (`stanrt_run`, process start to CSV) | 0.24 s |
| CmdStan | model build (stanc + clang, warm PCH) | 4.62 s |
| CmdStan | NUTS 1000+1000 (self-reported total) | 0.044 s |
| CmdStan | build + run wall time | ~4.98 s |

Time-to-first-draw is ~20x faster (0.24 s vs ~4.98 s). Sampling-phase
times are dominated by gradient cost, and adaptation trajectories differ
between engines (CmdStan took 9,654 post-warmup leapfrogs here), so treat
the sampling rows as indicative; the controlled comparison is the
per-gradient table.

## Numerical parity

Every model in the passing set is differentially verified against
CmdStan's `log_prob_propto_jacobian` and full gradient at the shared
point: 118/120 verified, 44 of them bitwise identical, worst relative
deviation 2.6e-12 (`tools/verify_sample.py`, `docs/corpus-status.md`).
Re-rolled models change summation order relative to CmdStan's scalar
loop, so they verify at tolerance rather than bitwise: across the corpus
the pass touches 6 models and the worst gradient deviation it introduces
vs the unrolled graph is 4.1e-15 relative (`spikes/ab_corpus.py`
compares every corpus model pass-on vs pass-off and flags any
divergence).

## Reproducing

```
./tools/dev_setup.sh --corpus          # deps, build, posteriordb, CmdStan
cmake -B build-rel -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel -j
python3 tools/bench_models.py deps/cmdstan deps/posteriordb
python3 spikes/ab_corpus.py deps/posteriordb   # re-roll A/B over the corpus
```
