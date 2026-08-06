# Changelog

## 0.1.0

First public release.

- Stan models compiled and sampled with no C++ toolchain on the machine:
  the real stanc3 is linked into the shared library, models lower to an op
  graph over precompiled stan-math kernels, and the graph doubles as the
  autodiff tape.
- NUTS with diagonal-metric adaptation (`stan::mcmc::adapt_diag_e_nuts`),
  at CmdStan's max tree depth of 10.
- 118 of 120 posteriordb models differentially verified against CmdStan on
  the log density and every gradient component; 44 bitwise identical,
  worst deviation 2.6e-12 relative.
- Per-gradient latency 1.1x to 6.2x faster than CmdStan on nine of the ten
  benchmark models, 0.53x on `low_dim_gauss_mix`. Time to first draw
  roughly 20x faster, since there is no compile step.
- Graph passes: loop re-rolling turns unrolled per-observation loops back
  into vectorized ops (`radon_pooled` goes from 27,670 ops to 8),
  destructive functional updates, store-to-load forwarding, and dead-write
  sweeping. `STANLI_NO_REROLL=1` disables re-rolling.
- Transformed parameters and generated quantities are computed by a
  second forward-only graph and written by the command line tool for 93 of
  the 119 compiling corpus models.
- Wheels for macOS arm64 and x86_64, Linux x86_64 and aarch64
  (manylinux_2_28). 13.8 MB installed.

Known gaps in the Python API: `sample()` returns declared parameters only,
so transformed parameters and generated quantities are not surfaced yet.
No variational inference, no optimization, no multi-chain threading, no
convergence diagnostics, no Windows wheel.
