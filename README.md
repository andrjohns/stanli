# stanrt

A portable Stan runtime: op-graph executor over precompiled stan-math
kernels. No C++ toolchain, no LLVM, no compilation on the user's machine.

Design: `docs/superpowers/specs/2026-08-04-stan-portable-runtime-design.md`
Current plan: `docs/superpowers/plans/2026-08-04-m1-spine.md`

## Python

```
./tools/build_wheel.sh          # builds dist/stanrt-*.whl for this platform
pip install dist/stanrt-*.whl
```

```python
import stanrt
m = stanrt.Model(stan_file="model.stan", data="data.json")
draws = m.sample(seed=1, warmup=1000, samples=1000)
draws["mu"].mean()
```

The wheel bundles one file: the stanrt shared library (C ABI in
`runtime/include/stanrt/capi.h`) with stanc3 embedded in-process (OCaml
compiled to a self-contained object via tools/stanc_embed/, linked in).
Model compilation, lowering, and sampling all happen inside the library;
no subprocess, nothing compiled on the user's machine. Builds without the
embed object fall back to running a bundled stanc binary as a subprocess.

## Build

```
./deps/fetch.sh
cmake -B build
cmake --build build -j
ctest --test-dir build
```

## Status

Milestone 1 (spine) complete on macOS arm64 / clang. 9/9 tests green.

| Proven | How |
| --- | --- |
| Recorder scalar reproduces stan-math gradients | Bitwise vs the var path across 7 densities, mixed data/parameter shapes (`test_densities`, `test_recorder`) |
| Zero-copy promotion of double buffers to rvar views | Static layout asserts + bitwise equality of copied vs mapped inputs |
| Graph executor forward/reverse | Bitwise vs closed form and var references (`test_executor`) |
| Native ops with hand vjps (exp, add_n, bcast_fma, matvec) | Bitwise vs var tape, including accumulation-order matching |
| Legacy op mechanism (nested var tape replay) | log_sum_exp and softmax bitwise vs var path (`test_legacy`) |
| Whole-model parity | Eight schools (10-dim) and logistic GLM (4-dim): log_prob + full gradient bitwise vs all-var references at fixed points |
| Sampling | stan::mcmc::adapt_diag_e_nuts over the executor gradient via a one-vari precomputed_gradients adapter; statistical checks on a 10-dim standard normal and eight schools (`test_nuts`) |

Notes:

- FP contraction is pinned off project-wide; clang otherwise forms FMAs
  differently across template instantiations of the same math, which breaks
  bitwise comparisons (measured 2 ULP before pinning).
- All densities are instantiated propto=false (target += semantics).
  propto variants arrive with the stanc3 backend in M2.
- Kernels bind every argument as rvar; comparisons against mixed
  data/parameter var instantiations can differ by ULPs through stan-math's
  to_ref_if caching (bounded and tested; see test_densities).

M2 (compiler) status: stanc3 --debug-transformed-mir sexp -> graph compiler
with for-loop unrolling, Single indexing, constraint transforms
(lower/upper/lower-upper), a compile-time transformed-data interpreter, JSON
data loading, stanrt_run/stanrt_check CLIs. 67/120 posteriordb models
compile and evaluate (docs/corpus-status.md). Eight schools runs end to end
from .stan + .json through NUTS with bitwise gradient parity vs the var
path at fixed points.

Remaining M2 work: CmdStan gradient-reference fixtures for the passing set
(the deps/cmdstan build for that rig exists), remaining transforms
(simplex, ordered, cholesky_corr), matrix ops, log_prob-side indexed
assignment cases, ODE/GP models.
