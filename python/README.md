# stanrt

Compile and sample Stan models with no C++ toolchain on the machine.

`pip install stanrt` gives you one shared library containing the Stan
compiler, the Stan math library, and Stan's NUTS sampler. Models are
turned into a graph of precompiled operations at load time, so preparing a
model takes milliseconds instead of a multi-second C++ compile, and
nothing is built on the user's machine.

```python
import stanrt

model = stanrt.Model(stan_file="eight_schools.stan", data="data.json")
draws = model.sample(seed=1, warmup=1000, samples=1000)
draws["mu"].mean()
```

## What it is

Every Stan model is a composition of a fixed vocabulary of operations:
densities, constraint transforms, linear algebra, elementwise math. stanrt
ships those precompiled and turns each model into data — a static graph
over flat buffers — rather than generating and compiling C++ per model.
The graph doubles as the autodiff tape: the forward sweep computes the log
density, the reverse sweep contracts adjoints, and steady-state gradient
evaluation allocates nothing.

The real stanc3 compiler is linked in, so the Stan language is supported
as the official toolchain supports it, and every kernel is the unmodified
stan-math code CmdStan itself runs.

## Correctness

118 of 120 posteriordb models are differentially verified against CmdStan:
same model, same data, same evaluation point, comparing log density and
every gradient component. 44 of them agree bitwise; the worst deviation
across the whole set is 2.6e-12 relative.

## Performance

Per-gradient latency against CmdStan on the same models: 1.9x-3.0x faster
on models written with vectorized statements, 0.4x-0.9x on models written
as explicit per-observation loops (those unroll, and the per-operation
overhead adds up). Time-to-first-draw is roughly 20x faster, because there
is no compile step.

## Status

Early. The API is small and will grow; the numbers above are reproducible
from the repository. macOS arm64 wheels first, Linux to follow.
