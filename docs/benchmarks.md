# Benchmarks vs CmdStan

2026-08-06, Apple M-series (macOS arm64), clang, both sides -O3 and
-ffp-contract=off, single-threaded. Both engines evaluate the sampling
gradient (propto + jacobian) at the same deterministic unconstrained
point. stanli runs `tools/bench_grad.cpp`; CmdStan runs
`tools/bench_cmdstan_grad.cpp` compiled against the stanc-generated
header, looping the same fresh-vars + grad + recover_memory cycle
`stan::model::gradient` performs per leapfrog step. Reproduce the whole
table with `tools/bench_models.py deps/cmdstan deps/posteriordb`.

## Per-gradient latency

| model | unconstrained params | stanli ns/grad | CmdStan ns/grad | speedup | stanli prep | CmdStan build |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `radon_pooled` | 3 | 53108 | 328107 | 6.18x | 0.081 s | 6.4 s |
| `radon_hierarchical_intercept_centered` | 391 | 112087 | 574179 | 5.12x | 0.174 s | 6.9 s |
| `arK` | 7 | 2458 | 11860 | 4.83x | 0.007 s | 6.6 s |
| `radon_county_intercept` | 388 | 90357 | 431480 | 4.78x | 0.128 s | 6.6 s |
| `bym2_offset_only` | 3845 | 40023 | 125379 | 3.13x | 0.047 s | 7.8 s |
| `eight_schools_noncentered` | 10 | 281 | 638 | 2.27x | 0.004 s | 6.9 s |
| `kidscore_momiq` | 3 | 1906 | 3867 | 2.03x | 0.006 s | 6.5 s |
| `lsat_model` | 1006 | 46571 | 91202 | 1.96x | 0.053 s | 7.3 s |
| `wells_dist100ars_model` | 3 | 17341 | 18393 | 1.06x | 0.026 s | 6.8 s |
| `low_dim_gauss_mix` | 5 | 186415 | 98905 | 0.53x | 0.196 s | 6.8 s |

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
from 27,670 ops to 8 (0.91x -> 6.18x) and `arK` from 3,164 to 21 (0.40x
-> 4.83x). Indexed reads rewrite by shape: the whole base in order needs
no op at all, a contiguous window becomes an `OP_SLICE`, and an arbitrary
data-driven index — `alpha[county_idx[n]]`, the hierarchical idiom,
repeats and all — becomes one `OP_GATHER` whose backward scatter-adds.
Anything the pass cannot prove safe it leaves alone, per region:
cross-lane recurrences, outputs escaping their lane, opcodes outside its
vocabulary. Set `STANLI_NO_REROLL=1` to disable it, `STANLI_NO_INPLACE=1`
for the update rules below.

**Element writes: `mu[n] = ...` inside a loop.** This is the other half
of the hierarchical idiom, and it used to be the worst thing in the
project. Each write lowered to a *functional update* — copy the whole
vector into a fresh slot, poke one element — so N writes cost O(N^2) time
and O(N^2) arena. `radon_county_intercept` (N=12,573) spent 90.5 ms per
gradient inside 2.58 GB of arena, 207x slower than CmdStan.

Three rules compose to remove it (`runtime/src/inplace.cpp`, plus the
index rules above). A write may mutate its vector directly when it is the
**last use** of that vector — not merely its only use, since the
read-back in the same iteration is an earlier use — and when no earlier
reader needs the vector's values during the reverse sweep (`log_sum_exp`
and the other nested-replay backwards rebuild their tape from the input
buffer, so they must find it intact). The write and its read-back then
cancel outright, and when nothing else reads the vector its writes are
dead and swept. What is left is plain per-lane arithmetic, which the
gather rule vectorizes: **77,960 ops become 9**, 90.5 ms becomes 92 us,
2.58 GB becomes 42 MB. Seven radon-family models and `rats_model` collapse
the same way.

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

Preparation time is the other axis: stanli lowers a model in 4-200 ms,
against a 6.2-7.6 s CmdStan compile (with a warm precompiled header, and
after a multi-minute one-time `make build`). That gap is what
time-to-first-draw is made of. Re-rolling also cut preparation time on
loop-heavy models (radon_pooled 0.39 s -> 0.08 s): the executor binds
and sizes 8 ops instead of 27,670.

## End to end: eight schools, model.stan + data.json -> 1000 warmup + 1000 draws

| engine | stage | time |
| --- | --- | --- |
| stanli | stanc + graph compile + bind | 0.014 s |
| stanli | NUTS 1000+1000 (incl. constraining draws) | 0.020 s |
| stanli | CLI total (`stanli_run`, process start to CSV) | 0.24 s |
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
the passes change 28 models and the worst gradient deviation any of them
introduces vs the untransformed graph is 3.7e-14 relative
(`spikes/ab_corpus.py` compares every corpus model passes-on vs
passes-off and flags any divergence; `--disable` one variable to
attribute one).

That harness earns its keep. An earlier version of the in-place rule
allowed a destructive write whenever it was the last use of its vector,
which is wrong for any earlier reader that rebuilds its var tape from the
buffer during the reverse sweep — `log_sum_exp`, `softmax`, every legacy
nested-replay backward. Eight HMM/LDA/mixture models were silently wrong
by up to 1.7e+05 relative **with their op counts unchanged**, so nothing
structural would have caught it. Only ops whose backward purely routes
adjoints may now precede a destructive write.

All six were then re-run through the CmdStan rig directly rather than
resting on that transitive argument, and all six still verify:
`radon_pooled` 2.1e-14 (140 ulp, against 135 before the pass), `arK`
9.6e-16 (5 ulp, against 1), `rats_model` 2.4e-16 (2 ulp),
`soil_incubation` 1.3e-16 (1 ulp), and both `covid19imperial` variants
8.2e-16 (7 ulp), unchanged.

## Reproducing

```
./tools/dev_setup.sh --corpus          # deps, build, posteriordb, CmdStan
cmake -B build-rel -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel -j
python3 tools/bench_models.py deps/cmdstan deps/posteriordb
python3 spikes/ab_corpus.py deps/posteriordb   # re-roll A/B over the corpus
```
