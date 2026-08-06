# Changelog

## 0.1.0

First public release.

- Stan models compiled and sampled with no C++ toolchain on the machine:
  the real stanc3 is linked into the shared library, models lower to an op
  graph over precompiled stan-math kernels, and the graph doubles as the
  autodiff tape.
- NUTS with diagonal-metric adaptation (`stan::mcmc::adapt_diag_e_nuts`).
- 118 of 120 posteriordb models differentially verified against CmdStan on
  the log density and every gradient component; 44 bitwise identical, worst
  deviation 2.6e-12 relative.
- Per-gradient latency 1.1x to 6.2x faster than CmdStan on seven of the
  eight benchmark models, 0.53x on `low_dim_gauss_mix`. Time to first draw
  roughly 20x faster, since there is no compile step.
- Loop re-rolling pass: unrolled per-observation loops are detected as
  periodic op regions and rewritten into vectorized ops. `radon_pooled`
  goes from 27,670 ops to 8. Set `STANLI_NO_REROLL=1` to disable.
- Wheels for macOS arm64 and x86_64, Linux x86_64 and aarch64
  (manylinux_2_28). 13.8 MB installed.

Known gaps: no generated quantities, no variational inference or
optimization, no multi-chain threading, no Windows wheel.
