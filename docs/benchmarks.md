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

**ODE models: the right-hand side was an interpreter.** Every user function
is inlined at lowering time except one -- an ODE right-hand side has to stay
callable at runtime, because the integrator picks the times. It was evaluated
by a tree-walking interpreter over the MIR, at a `std::map` lookup per
variable reference and a `std::vector` allocation per intermediate: 5.8 us per
call for lotka_volterra's two-line right-hand side, ~500 calls per gradient,
**97% of the model's gradient time**. It also solved the system twice per
gradient, once for the values and again for the derivatives.

Both are gone. The right-hand side compiles once, at lowering time, into a
flat register machine (`runtime/src/ode_prog.cpp`): names become indices,
loops over the states unroll, data-only conditions fold, conditions on the
solve time become branches, and a call is a switch over a contiguous
instruction array with no allocation. And the forward sweep, which has to
solve the coupled state-plus-sensitivity system anyway to match CmdStan's
step control, now keeps the sensitivities instead of throwing them away, so
the backward is a matrix-vector product.

| model | before | after | speedup | vs CmdStan |
| --- | ---: | ---: | ---: | ---: |
| `lotka_volterra` | 2,790,941 ns | 71,704 ns | 38.9x | 0.015x -> 0.58x |
| `soil_incubation` | 3,389,538 ns | 96,362 ns | 35.2x | 0.018x -> 0.63x |
| `one_comp_mm_elim_abs` | 18,873,857 ns | 653,181 ns | 28.9x | - |

Gradients are unchanged to the bit where they were before, and
`lotka_volterra` moved from 4 ULP to bitwise identical to CmdStan: reading the
jacobian out of the same solve that produced the values removes a second,
independently stepped solve. All four corpus models that call
`integrate_ode_*` compile their right-hand side. Anything the compiler cannot
express -- a `return` out of a branch on the solve time, say -- keeps the
interpreter, so coverage never shrinks; `STANLI_DEBUG_ODE=1` reports when that
happens, since a silent 30x is worth a line.

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

**The sampling rows below, and the sampling columns of
`docs/corpus-bench.tsv`, were measured with a defective max tree depth of
5** (`stan::mcmc::base_nuts` defaults to 5; CmdStan sets 10, and
`run_nuts` never called `set_max_depth`). Trajectories capped at 31
leapfrogs instead of 1023, so any model needing deep trees was
under-explored and its sampling time understated -- `prophet` ran 61,510
gradient evaluations in 6.4 s at depth 5 against 1,686,852 in 176.5 s at
the correct depth, versus CmdStan's 974,872 post-warmup leapfrogs in
117.7 s. The per-gradient and preparation columns are unaffected. Numbers
here are pending a re-run.

They were also unequal work in a second way: stanli computed no
transformed parameters and no generated quantities, so on the models with
a generated quantities block (`diamonds`, `accel_splines`,
`covid19imperial_v2` among the biggest apparent sampling wins) CmdStan
was doing per-draw work stanli skipped. The write_array graph closes that
for 93 of the 119 compiling models; the rest stop at an RNG call in
generated quantities, which stanli cannot yet evaluate.

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
