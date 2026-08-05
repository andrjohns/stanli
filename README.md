# stanrt

A portable Stan runtime: op-graph executor over precompiled stan-math
kernels. No C++ toolchain, no LLVM, no compilation on the user's machine.

- Performance vs CmdStan: [docs/benchmarks.md](docs/benchmarks.md)
  (sampling gradient 1.26x faster, time-to-first-draw ~20x faster)
- Model coverage: [docs/corpus-status.md](docs/corpus-status.md)
  (81/120 posteriordb models, every one differentially verified against
  CmdStan's log_prob and gradients)
- Design doc: `docs/superpowers/specs/2026-08-04-stan-portable-runtime-design.md`

## Architecture

The premise: a Stan model does not need machine code generated for it.
Every model is a composition of a fixed vocabulary of operations
(densities, constraint transforms, linear algebra, elementwise math), so
stanrt ships those operations precompiled and turns each model into data:
a static graph of ops over flat buffers, built at load time and executed
by a small interpreter. There is no JIT and no C++ codegen; "compiling" a
model takes milliseconds.

```
model.stan + data.json
  |  stanc3 (official OCaml compiler, linked into the library)
  v
transformed MIR (s-expression)
  |  lowering: runtime/src/lower.cpp
  v
op graph + preallocated value/adjoint arenas
  |  executor: forward = log density, reverse = gradient
  v
NUTS (stan::mcmc::adapt_diag_e_nuts) -> draws
```

Stage by stage:

1. **stanc3, in process.** The real Stan compiler (OCaml) is compiled to
   a self-contained object (`-output-complete-obj`) and linked into the
   shared library. It parses, typechecks, and optimizes the model, and
   stanrt consumes its transformed MIR directly. Full language fidelity
   without a subprocess or a vendored parser rewrite.

2. **Lowering** (`runtime/src/lower.cpp`). A compile-time interpreter
   walks the MIR against the actual data: transformed data is evaluated
   eagerly, loops with data-known bounds are unrolled, and the model
   block flattens into a linear sequence of ops reading and writing
   preallocated arenas. `~` statements lower to the same propto +
   per-argument-activity instantiations CmdStan's generated C++ uses, so
   dropped constants and skipped data partials match exactly.

3. **Execution** (`runtime/src/executor.cpp`). The op graph is the AD
   tape. The forward sweep computes the log density and stashes each
   op's partials in per-op scratch; the reverse sweep runs the ops
   backward, contracting adjoints. Steady-state gradient evaluation
   performs zero allocation, which is where the speedup over the
   pointer-chasing var tape comes from.

4. **Kernels** (`runtime/kernels/`). Two tiers behind one interface.
   Native kernels are hand-written forward/backward pairs that mirror
   the exact Eigen expressions of stan-math's rev overloads, so
   gradients match CmdStan bitwise (FP contraction pinned off
   project-wide). Everything else runs as a "legacy" op: a recorder
   scalar (`rvar`, a registered stan-math scalar type) or a nested var
   tape replay drives unmodified stan-math prim/prob templates and
   deposits values and partials into the caller's buffers. Legacy ops
   make the whole library expressible; native kernels make the hot path
   fast. Both are compiled once, when the stanrt binary is built.

5. **Sampling** (`runtime/src/nuts.cpp`). Stan's own NUTS with
   diagonal-metric adaptation, driven through a thin model adapter that
   returns one precomputed-gradients vari per evaluation.

6. **Distribution.** Everything above sits behind a C ABI
   (`runtime/include/stanrt/capi.h`) in one shared library; the Python
   package is a ctypes wrapper around it. A platform wheel is one .whl
   containing one dylib.

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

Builds without the embedded stanc3 object fall back to running a bundled
stanc binary as a subprocess.

## Build

One-shot setup (fetches pinned deps, builds, runs tests):

```
./tools/dev_setup.sh            # core build + tests
./tools/dev_setup.sh --embed    # + OCaml toolchain, in-process stanc3
./tools/dev_setup.sh --corpus   # + posteriordb and the CmdStan verify rig
./tools/dev_setup.sh --all
```

Or manually:

```
./deps/fetch.sh
cmake -B build
cmake --build build -j
ctest --test-dir build
```

## Verification policy

Nothing ships on "looks close". Kernel gradients are bitwise-tested
against stan-math's var path at fixed points; whole models are
differentially verified against CmdStan (same generated model, same
deterministic evaluation point, `tools/verify_sample.py`). The corpus
scoreboard (`tools/corpus.py`) tracks which posteriordb models compile,
evaluate, and verify. Details in
[docs/corpus-status.md](docs/corpus-status.md).

## Status

macOS arm64 / clang: 16/16 tests green; 81/120 posteriordb models
passing, all CmdStan-verified. Sampling-semantics gradients (propto with
per-argument activity) landed; see [docs/benchmarks.md](docs/benchmarks.md)
for the numbers.

In progress: the remaining corpus models (cholesky transforms, GP
covariance ops, ODE integrators, a few indexing forms), Linux wheels +
CI, the CRAN shim.
