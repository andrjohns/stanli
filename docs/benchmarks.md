# Benchmarks: eight schools end to end vs CmdStan

2026-08-05, Apple M-series (macOS arm64), clang, both sides -O3,
single-threaded. Model: `tests/fixtures/es.stan` (non-centered eight
schools, 10 unconstrained parameters) with the posteriordb dataset.
CmdStan: current develop, built with its own defaults plus the
precompiled-header warm path. Harnesses: `tools/bench_grad.cpp` (stanrt)
and an equivalent driver looping CmdStan's generated model through the same
fresh-vars + grad + recover_memory cycle `stan::model::gradient` performs
per leapfrog.

## Per-gradient latency (200,000 evals at a fixed point)

| engine | semantics | ns/eval | vs stanrt |
| --- | --- | --- | --- |
| stanrt | full lpdf + jacobian | 526 | 1.00x |
| CmdStan | full lpdf + jacobian (log_prob_jacobian) | 518 | 0.98x |
| CmdStan | propto + jacobian (sampling path) | 344 | 0.65x |

At matched semantics stanrt is at parity with the compiled model (the op
graph replaces the var tape: no per-node allocation, no tape teardown,
which pays for the interpretive dispatch). CmdStan's sampling path is 1.53x
faster than stanrt today because `~` statements drop parameter-constant
terms (propto=true); stanrt lowers everything propto=false. Propto kernel
variants close that gap and are planned (gradients are unaffected; only
dropped constant terms differ).

## End to end: model.stan + data.json -> 1000 warmup + 1000 draws

| engine | stage | time |
| --- | --- | --- |
| stanrt | stanc (subprocess) + graph compile + bind | 0.014 s |
| stanrt | NUTS 1000+1000 (incl. constraining draws) | 0.020 s |
| stanrt | CLI total (`stanrt_run`, process start to CSV) | 0.24 s |
| CmdStan | model build (stanc + clang, warm PCH) | 4.62 s |
| CmdStan | NUTS 1000+1000 (self-reported total) | 0.044 s |
| CmdStan | build + run wall time | ~4.98 s |

Time-to-first-draw is ~20x faster (0.24 s vs ~4.98 s), and the 4.62 s
CmdStan build is the best case: it assumes `make build` has already
produced CmdStan's precompiled header and libraries (a multi-minute
one-time cost), and eight schools is a small model; larger models take
substantially longer to compile while stanrt's lowering stays in
milliseconds.

Sampling-phase times are comparable and dominated by gradient cost;
adaptation trajectories and leapfrog counts differ between engines
(CmdStan took 9,654 post-warmup leapfrogs here), so treat the sampling
rows as indicative rather than a controlled comparison. The controlled
comparison is the per-gradient table.

## Reproducing

```
cmake -B build-rel -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel --target bench_grad
./deps/stanc3/stanc --debug-transformed-mir tests/fixtures/es.stan > /tmp/es.sexp
./build-rel/bench_grad /tmp/es.sexp tests/fixtures/eight_schools.json 200000
```

The CmdStan side needs a CmdStan checkout with `make build` done; compile
the generated model header against the driver in the git history of this
file (tools/ directory) with -O3 and link stan-math's TBB.
