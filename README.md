# stanli

The Stan Language Interpreter: an op-graph executor over precompiled
stan-math kernels. No C++ toolchain, no LLVM, no compilation on the
user's machine.

- Performance vs CmdStan: [docs/benchmarks.md](docs/benchmarks.md)
  (gradient 1.1x-6.2x faster on seven of the eight benchmark models,
  0.53x on the one that still defeats the re-roll pass;
  time-to-first-draw ~20x faster)
- Model coverage: [docs/corpus-status.md](docs/corpus-status.md)
  (118/120 posteriordb models differentially verified against CmdStan's
  log_prob and full gradient, 44 of them bitwise identical, worst
  relative deviation 2.6e-12; per-model accuracy in relative terms and
  ULPs is listed there)
- Install size: one 13.8 MB shared library, a 4.9 MB wheel. Breakdown
  in [Binary size](#binary-size) below.
- Design doc: `docs/superpowers/specs/2026-08-04-stan-portable-runtime-design.md`

## Architecture

The premise: a Stan model does not need machine code generated for it.
Every model is a composition of a fixed vocabulary of operations
(densities, constraint transforms, linear algebra, elementwise math), so
stanli ships those operations precompiled and turns each model into data:
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
   stanli consumes its transformed MIR directly. Full language fidelity
   without a subprocess or a vendored parser rewrite.

2. **Lowering** (`runtime/src/lower.cpp`). A compile-time interpreter
   walks the MIR against the actual data: transformed data is evaluated
   eagerly, loops with data-known bounds are unrolled, and the model
   block flattens into a linear sequence of ops reading and writing
   preallocated arenas. `~` statements lower to the same propto +
   per-argument-activity instantiations CmdStan's generated C++ uses, so
   dropped constants and skipped data partials match exactly.

3. **Re-rolling** (`runtime/src/reroll.cpp`). Unrolling is what makes
   the graph concrete, but a model written as a per-observation loop
   arrives as N copies of one small op template, and the interpreter's
   cost is per op. This pass finds those periodic regions and rewrites
   them into the vectorized ops the kernels already support: constant
   vectors materialized from the const pool, invariant ops hoisted,
   elementwise lanes widened, index progressions collapsed into their
   base vector, and N scalar density terms fused into one summed vector
   density. `radon_pooled` goes from 27,670 ops to 8. Whatever the pass
   cannot prove safe it leaves alone, one region at a time.
   `STANLI_NO_REROLL=1` disables it.

4. **Execution** (`runtime/src/executor.cpp`). The op graph is the AD
   tape. The forward sweep computes the log density and stashes each
   op's partials in per-op scratch; the reverse sweep runs the ops
   backward, contracting adjoints. Steady-state gradient evaluation
   performs zero allocation, which is where the speedup over the
   pointer-chasing var tape comes from.

5. **Kernels** (`runtime/kernels/`). Two tiers behind one interface.
   Native kernels are hand-written forward/backward pairs that mirror
   the exact Eigen expressions of stan-math's rev overloads, so
   gradients match CmdStan bitwise (FP contraction pinned off
   project-wide). Everything else runs as a "legacy" op: a recorder
   scalar (`rvar`, a registered stan-math scalar type) or a nested var
   tape replay drives unmodified stan-math prim/prob templates and
   deposits values and partials into the caller's buffers. Legacy ops
   make the whole library expressible; native kernels make the hot path
   fast. Both are compiled once, when the stanli binary is built.

6. **Sampling** (`runtime/src/nuts.cpp`). Stan's own NUTS with
   diagonal-metric adaptation, driven through a thin model adapter that
   returns one precomputed-gradients vari per evaluation.

7. **Distribution.** Everything above sits behind a C ABI
   (`runtime/include/stanli/capi.h`) in one shared library; the Python
   package is a ctypes wrapper around it. A platform wheel is one .whl
   containing one dylib.

## Binary size

One self-contained shared library, 13.8 MB installed, 4.9 MB compressed
in the wheel. Attributing its 12.7 MB of code and data by symbol:

| | | |
| --- | ---: | ---: |
| embedded stanc3 (all OCaml) | 6.04 MB | 47.7% |
| stan-math | 5.59 MB | 44.2% |
| stanli itself | 0.39 MB | 3.0% |
| Eigen (out-of-line) | 0.27 MB | 2.2% |
| SUNDIALS | 0.17 MB | 1.3% |
| Boost, nlohmann/json, NUTS, libc++, unattributed | 0.20 MB | 1.6% |

The interpreter and NUTS together are about 410 KB. Nearly all of the
rest is the Stan compiler and the math library, which is the trade the
design makes: ship every kernel and the compiler once so that nothing is
built on the user's machine.

Within the OCaml half, stanc3's own modules are 2.74 MB and its
dependencies 3.01 MB (Jane Street Base and Core 1.66 MB, OCaml stdlib
1.07 MB, Yojson and Menhir 0.28 MB). The OCaml C runtime itself is only
290 KB, and a third of that is link tables rather than code.

Two things the linker cannot do here. `ocamlopt` does not set
`MH_SUBSECTIONS_VIA_SYMBOLS` on its output (and this switch was built
without `--enable-function-sections`), so `-dead_strip` cannot reach
inside the compiler object at all: the whole 11 MB is one indivisible
atom, and stripping recovers 117 KB, all of it from the C++ side.
Module-level selection does work, and already happens at link time
(`base.a` is 3.8 MB on disk, of which 1.6 MB is linked).

Rebuilding the compiler with `--enable-function-sections` would not buy
much either, and the ceiling is measurable without building it: model the
linker at its finest possible granularity, one node per defined symbol
with an edge wherever a relocation inside one symbol names another, and
96.4% of the object is reachable from its entry points. Only 220 KB of
6.06 MB is not, most of it module initializers a linker keeps anyway.
OCaml's module blocks hold closures for every top-level function in the
module, so function-level stripping has almost nothing to bite on.

Bytecode is the one large lever measured so far: `(modes (byte object))`
produces a 10.5 MB library, 3.7 MB smaller. Three effects, and the
largest is not the one folklore expects: global static data is marshalled
rather than laid out as a relocated object graph (3.01 MB to 984 KB),
per-function unwind and exception metadata disappears (531 KB of
`__eh_frame` to 828 bytes), and code compresses only 1.35x (2.79 MB of
ARM64 to 2.06 MB of bytecode). The interpreter itself costs 11 KB. The
price is roughly 8x slower model compilation, 2 ms to 18 ms for eight
schools, and that trade is not taken.

## Python

A ctypes wrapper over the same shared library, published to PyPI as one
platform wheel per platform.

```
pip install stanli                 # or: ./tools/build_wheel.sh
```

```python
import stanli
m = stanli.Model(stan_file="model.stan", data={"J": 8, "y": y, "sigma": s})
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

macOS arm64 / clang: 17/17 tests green; 119/120 posteriordb models
compile and evaluate, 118 of them CmdStan-verified. Of the two that are
not: `sir`'s ODE solution dips ~1e-9 below a declared lower bound at
every shared evaluation point and CmdStan rejects it there too, and
`kronecker_gp` matches on lp and 436 of 438 gradients but differs on the
two that flow through eigenvectors of a nearly degenerate covariance
(see the note in the corpus status). Sampling-semantics gradients (propto with
per-argument activity) landed; see [docs/benchmarks.md](docs/benchmarks.md)
for the numbers.

## Roadmap

1. Widening the re-roll pass. It now handles the shapes where every
   lane's density output feeds the target directly, which covered
   `radon_pooled` and `arK`. The remaining benchmark loser,
   `low_dim_gauss_mix` (0.53x), writes `log_mix(theta, normal_lpdf(...),
   ...)` per observation, so the density outputs feed an op instead of
   the target and the pass correctly refuses. Closing it needs an
   elementwise-lp density variant plus batched `log_sum_exp`/`log_mix`
   kernels. Unary math opcodes (exp, log, inv_logit) are also outside
   the widening vocabulary today, which bounds how many corpus models
   the pass can reach. Fusing adjacent elementwise chains into one pass
   over the arena is the follow-on.
2. Linux + x86 wheels with CI owning the opam + cmake build
   (cibuildwheel); CRAN shim package.
3. Vectorized kernels via stan-math's varmat (SoA) overloads. Today the
   kernels mirror CmdStan's default AoS arithmetic, which is scalar for
   transcendentals and reductions (strided var access defeats Eigen's
   packet math). stan-math's `var_value<Matrix>` overloads compute over
   contiguous doubles and vectorize, and `stanc --O1` already emits them
   variable-by-variable where every use is varmat-compatible. The plan
   follows the same shape: switch kernels to mirror the varmat
   expressions function-by-function where the overload exists
   (constrains, elementwise, matvec, the common densities), verify
   differentially against `stanc --O1` CmdStan builds, and keep AoS
   parity for the rest. Profile a large-N model first to size the win;
   graph-level fusion of elementwise chains is the follow-on.
