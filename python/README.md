# stanli

**The Stan Language Interpreter.** Compile and sample Stan models with no
C++ toolchain on the machine.

[![PyPI](https://img.shields.io/pypi/v/stanli.svg)](https://pypi.org/project/stanli/)
[![Python](https://img.shields.io/pypi/pyversions/stanli.svg)](https://pypi.org/project/stanli/)
[![License](https://img.shields.io/pypi/l/stanli.svg)](https://github.com/seantalts/stanli/blob/main/LICENSE)
[![wheels](https://github.com/seantalts/stanli/actions/workflows/wheels.yml/badge.svg)](https://github.com/seantalts/stanli/actions/workflows/wheels.yml)

```console
pip install stanli
```

That is the whole install. No compiler, no `make`, no CmdStan checkout, no
multi-minute first-run build. One wheel, one shared library, roughly five
megabytes.

```python
import stanli

model = stanli.Model(stan_file="eight_schools.stan", data="data.json")
draws = model.sample(seed=1, warmup=1000, samples=1000)

draws["mu"].mean()      # one numpy array of draws per constrained parameter
```

Model preparation takes milliseconds, so the first draw arrives about 20x
sooner than a toolchain that compiles C++ per model.

## How it works

Every Stan model is a composition of a fixed vocabulary of operations:
densities, constraint transforms, linear algebra, elementwise math. stanli
ships those precompiled and turns each model into *data*, a static graph of
ops over flat preallocated buffers, instead of generating and compiling C++
per model.

```
model.stan + data.json
  |  stanc3, the official OCaml compiler, linked into the library
  v
transformed MIR
  |  lowering: transformed data evaluated eagerly, data-bound loops unrolled,
  |            then periodic regions re-rolled back into vectorized ops
  v
op graph over preallocated value/adjoint arenas
  |  forward sweep = log density, reverse sweep = gradient
  v
NUTS with diagonal-metric adaptation -> draws
```

The graph doubles as the autodiff tape, so a reverse sweep is a backwards
loop over an array rather than a walk through a pointer-chasing tape, and
steady-state gradient evaluation allocates nothing.

Two things are not reimplemented, which is what makes the results
trustworthy: the compiler is the real stanc3, linked in-process, so the
Stan language behaves as the official toolchain makes it behave; and the
math is unmodified stan-math, the same code CmdStan runs.

## Correctness

Nothing here ships on "looks close".

**118 of 120 posteriordb models** are differentially verified against
CmdStan: same model, same data, same evaluation point, comparing the log
density and every single gradient component. **44 agree bitwise.** The worst
deviation across the entire corpus is **2.6e-12 relative**.

The two exceptions are documented rather than hidden. `sir`'s ODE solution
dips about 1e-9 below a declared lower bound at the shared evaluation point,
where CmdStan rejects it too; `kronecker_gp` matches on the log density and
436 of 438 gradients, differing on the two that flow through eigenvectors of
a nearly degenerate covariance matrix.

Full per-model accuracy table:
[docs/corpus-status.md](https://github.com/seantalts/stanli/blob/main/docs/corpus-status.md)

## Performance

Per-gradient latency against CmdStan, same models, same evaluation point,
both sides `-O3` with FP contraction pinned off:

| model | params | stanli | CmdStan | speedup |
| --- | ---: | ---: | ---: | ---: |
| `radon_pooled` | 3 | 52.7 us | 325.9 us | **6.2x** |
| `arK` | 7 | 2.4 us | 12.0 us | **4.9x** |
| `bym2_offset_only` | 3845 | 40.1 us | 122.0 us | **3.0x** |
| `kidscore_momiq` | 3 | 1.9 us | 3.9 us | **2.1x** |
| `eight_schools_noncentered` | 10 | 0.28 us | 0.56 us | **2.0x** |
| `lsat_model` | 1006 | 46.2 us | 91.6 us | **2.0x** |
| `wells_dist100ars_model` | 3 | 17.3 us | 18.3 us | **1.1x** |
| `low_dim_gauss_mix` | 5 | 184.5 us | 98.2 us | 0.53x |

The wins come from op granularity. CmdStan's var tape allocates, walks, and
frees one node per scalar operation per leapfrog step; stanli pays a fixed
cost per *op*, and a vectorized statement over N elements amortizes that to
nothing. The one loss is honest and understood: `low_dim_gauss_mix` writes
`log_mix(theta, normal_lpdf(...), ...)` per observation, a shape the
re-rolling pass correctly refuses to vectorize today.

Method and full table:
[docs/benchmarks.md](https://github.com/seantalts/stanli/blob/main/docs/benchmarks.md)

## API

The surface is small on purpose.

```python
import stanli

# A path to a .stan file, or the model source directly.
model = stanli.Model(stan_file="model.stan", data="data.json")
model = stanli.Model(stan_code=src, data={"J": 8, "y": y, "sigma": sigma})

model.n_unconstrained               # length of the unconstrained vector
model.constrained_names             # ['mu', 'tau', 'theta.1', ...]

lp, grad = model.log_prob_grad(q)   # sampling log density and its gradient

draws = model.sample(seed=1, warmup=1000, samples=1000, delta=0.8)
draws["mu"]                         # ndarray of length `samples`
```

`data` accepts a path to a JSON file or a dict of Python scalars, lists, and
numpy arrays. `sample` returns one array of constrained draws per scalar
parameter, named the way CmdStan names them, so `theta` declared as
`vector[8]` arrives as `theta.1` through `theta.8`.

## Platforms

Wheels for macOS (arm64 and x86_64) and Linux (x86_64 and aarch64,
manylinux_2_28). Windows is not built yet; it needs a mingw-w64 toolchain,
because stan-math does not build under MSVC.

The installed library is 13.8 MB, which is the trade this design makes:
ship the compiler and every kernel once, so that nothing is ever built on
the user's machine. Roughly half of that is the embedded stanc3 and
somewhat under half is stan-math. The interpreter and NUTS together are
about 410 KB.

## Status

Early, and deliberately narrow. The sampler is Stan's own NUTS with
diagonal-metric adaptation; there is no variational inference, no
optimization, no multi-chain threading, and no generated quantities block
yet. What is here is verified against CmdStan model by model, and every
number on this page is reproducible from the repository.

- Source, issues, and roadmap:
  [github.com/seantalts/stanli](https://github.com/seantalts/stanli)
- License: BSD-3-Clause, matching Stan's own.
