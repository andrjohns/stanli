# Benchmarks vs CmdStan

2026-08-05, Apple M-series (macOS arm64), clang, both sides -O3 and
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
| `bym2_offset_only` | 3845 | 40136 | 118549 | 2.95x | 0.047 s | 8.1 s |
| `eight_schools_noncentered` | 10 | 306 | 599 | 1.96x | 0.005 s | 6.9 s |
| `kidscore_momiq` | 3 | 1968 | 3856 | 1.96x | 0.007 s | 6.7 s |
| `lsat_model` | 1006 | 46197 | 88912 | 1.92x | 0.054 s | 7.3 s |
| `wells_dist100ars_model` | 3 | 17467 | 18462 | 1.06x | 0.028 s | 6.8 s |
| `radon_pooled` | 3 | 366658 | 332018 | 0.91x | 0.439 s | 6.4 s |
| `low_dim_gauss_mix` | 5 | 191177 | 98785 | 0.52x | 0.202 s | 6.8 s |
| `arK` | 7 | 28420 | 11964 | 0.42x | 0.034 s | 6.7 s |

The spread is not noise, and it has one cause.

**Where stanrt wins (1.9x-3.0x): models written with vectorized
statements.** A `y ~ normal(mu, sigma)` over 3845 elements is one op over
flat arenas here versus a var tape that allocates and chases pointers per
element. Removing the tape is worth roughly 2x on small models and grows
with size: the 3845-parameter model is the biggest win in the set.

**Where stanrt loses (0.42x-0.91x): models written as explicit scalar
loops.** `arK`, `low_dim_gauss_mix`, and `radon_pooled` all loop over N
data points doing scalar work per iteration. stanrt unrolls those loops
at compile time, so each iteration becomes its own op with its own kernel
call, sink setup, and stan-math instantiation. Measured per-op cost is
~400 ns for a scalar `normal_lpdf` op, against a handful of cheap var
allocations in CmdStan's compiled loop. Per-op overhead dominates, and we
pay it N times.

This is the clearest optimization target the project has: recognizing
unrolled scalar loops and re-rolling them into vectorized ops (or fusing
adjacent elementwise chains into one pass over the arena) attacks the
losing column directly, and it matters more than SIMD inside kernels.
Vectorizing the kernels themselves via stan-math's varmat overloads (see
the README roadmap) helps the winning column further but does nothing for
these three.

Preparation time is the other axis: stanrt lowers a model in 5-440 ms,
against a 6.4-8.1 s CmdStan compile (with a warm precompiled header, and
after a multi-minute one-time `make build`). That gap is what
time-to-first-draw is made of, and it does not shrink with model size the
way compile time does.

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
point: 105/105 verified, 41 of them bitwise identical, worst relative
deviation 9.2e-14 (`tools/verify_sample.py`, `docs/corpus-status.md`).

## Reproducing

```
./tools/dev_setup.sh --corpus          # deps, build, posteriordb, CmdStan
cmake -B build-rel -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel -j
python3 tools/bench_models.py deps/cmdstan deps/posteriordb
```
