# How much faster is stanli?

Across 119 posteriordb models, stanli evaluates a gradient **2.92x faster
than CmdStan at the median**. It is at least as fast on 117 of the 119 models.
Because stanli does not build a native C++ binary for each model, the first
complete run is typically faster by more than the gradient ratio alone
suggests.

## Eight Schools: 3.2x faster gradients, roughly 100x faster to draws

The non-centered Eight Schools model is a useful first result because it is
small: there is little work over which either engine can hide overhead.

| measurement | stanli | CmdStan | speedup |
| --- | ---: | ---: | ---: |
| one gradient at the same point | 233 ns | 745 ns | **3.20x** |
| first 1,000 warmup + 1,000 draw run | 0.03 s | 3.4 s | **roughly 100x** |

The stanli run is the whole command, from Stan source through model loading,
sampling, and CSV output. The CmdStan total is its 3.2 s model build plus its
0.20 s run. The source timings are recorded to only two or one decimal places,
so the headline and the first-run table columns deliberately use approximate
ratios.

The gradient row is the controlled comparison: both engines evaluate the same
sampling gradient at the same deterministic unconstrained point. The complete
run is what a user waits for, but it is indicative rather than controlled
because small numerical differences can send NUTS down different adaptation
and leapfrog trajectories.

## Representative models

Here is a deliberately mixed slice of the corpus, sorted from the largest
gradient win to the losses. It includes IRT, regression, hierarchical,
mixture, Gaussian-process, state-space, HMM, GARCH, and ODE models. Lower times
are better; higher speedups are better.

| model | stanli gradient | CmdStan gradient | gradient speedup | stanli source-to-CSV | CmdStan build + run | approx. first-run speedup |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `gpcm_latent_reg_irt` | 121.762 us | 1.338 ms | 10.99x | 9.58 s | 166.9 s | ~17x |
| `dogs` | 7.196 us | 63.747 us | 8.86x | 0.53 s | 5.9 s | ~11x |
| `radon_pooled` | 45.888 us | 320.938 us | 6.99x | 0.58 s | 6.4 s | ~11x |
| `GLM_Poisson_model` | 401 ns | 2.008 us | 5.01x | 0.06 s | 3.4 s | ~57x |
| `state_space_stochastic_level_stochastic_seasonal` | 6.726 us | 26.320 us | 3.91x | 12.02 s | 44.4 s | ~3.7x |
| `eight_schools_noncentered` | 233 ns | 745 ns | 3.20x | 0.03 s | 3.4 s | ~110x |
| `logistic_regression_rhs` | 40.743 us | 113.106 us | 2.78x | 11.95 s | 20.8 s | ~1.7x |
| `soil_incubation` | 26.731 us | 60.871 us | 2.28x | 6.15 s | 16.1 s | ~2.6x |
| `normal_mixture` | 41.662 us | 88.239 us | 2.12x | 0.43 s | 3.6 s | ~8.4x |
| `lotka_volterra` | 20.843 us | 41.313 us | 1.98x | 2.16 s | 10.3 s | ~4.8x |
| `hmm_example` | 16.081 us | 27.145 us | 1.69x | 0.50 s | 5.0 s | ~10x |
| `garch11` | 6.996 us | 9.664 us | 1.38x | 0.20 s | 3.2 s | ~16x |
| `hierarchical_gp` | 42.189 us | 47.565 us | 1.13x | 24.64 s | 25.8 s | ~1.0x |
| `one_comp_mm_elim_abs` | 459.356 us | 470.681 us | 1.02x | 9.18 s | 14.5 s | ~1.6x |
| `diamonds` | 31.050 us | 31.497 us | 1.01x | 51.59 s | 51.9 s | ~1.0x |
| `gp_regr` | 5.593 us | 4.698 us | 0.84x | 0.09 s | 5.4 s | ~60x |
| `gp_pois_regr` | 5.279 us | 3.935 us | 0.75x | 1.60 s | 6.9 s | ~4.3x |

Across all 117 models that completed a full run in both engines, the median
source-to-CSV speedup is **about 6.8x**, including CmdStan's model build. 115 of 117
finish at least as fast in stanli. As above, gradient speed is the controlled
result; full-run speed also reflects the trajectory taken by each sampler.
## What tends to win, and where it does not

**The largest wins are models with repeated independent work.** Regressions,
GLMs, IRT models, and many hierarchical models spend most of their time doing
the same operation for many observations. stanli executes those regions as a
few vector operations and reuses the resulting autodiff graph. CmdStan builds
and tears down scalar autodiff tape nodes on every gradient evaluation.

**Dense kernels and sequential models land closer to parity.** If most of a
gradient is one large matrix operation, both engines spend their time in the
same stan-math kernel. HMM, ARMA, and GARCH recurrences depend on the previous
step, so they cannot become independent vector lanes. stanli still wins on the
measured examples, but by less.

**The ODE models moved from stanli's weakest results to some of its
strongest.** `one_comp_mm_elim_abs`, `soil_incubation`, and `lotka_volterra`
ran at 0.87-0.90x CmdStan in the 0.10.0 benchmarks; they are now 1.02-2.28x.
All three call the legacy `integrate_ode_bdf`/`integrate_ode_rk45` interface,
and the change coincides with this release's expanded function coverage
inside ODE right-hand sides (see the changelog). Both engines still use the
same Stan Math integrator.

**Two Gaussian-process models are now the only gradient losses.** `gp_regr`
and `gp_pois_regr`, at 0.75-0.84x CmdStan, are dominated by covariance-matrix
construction, the same dense-kernel work described above, and now land just
under parity instead of just over it.

## Parallel chains

Chains run concurrently by default, with one executor and RNG stream per
chain. On an intentionally sequential 200-step ordered-logistic model, eight
chains scaled like this:

| worker threads | 1 | 2 | 4 | 8 |
| --- | ---: | ---: | ---: | ---: |
| eight chains, wall time | 2.89 s | 1.59 s | 0.86 s | 0.49 s |

Parallelism does not change the draws: an eight-chain run is byte-identical to
the same chains run sequentially. This is checked across four models and
asserted in `tests/test_multichain.cpp` and `tests/test_python.py`.

## Numerical agreement

The performance results sit behind a differential oracle, not a separate
approximation. 118 of 120 posteriordb models are verified against CmdStan's
log density and complete gradient; 41 agree bitwise, and the worst relative
deviation is 2.6e-12. See [the per-model accuracy table](corpus-status.md) for
the two documented exceptions and every model's error bound.

## Full corpus

Every completed posteriordb model is below, in the same high-to-low gradient
order as the representative slice. `stanli source-to-CSV` times the complete
`stanli_run` process. `CmdStan build + run` adds the separately measured model
build and sampling command. Both runs use 1,000 warmup iterations, 1,000 draws,
and seed 1.

| model | stanli gradient | CmdStan gradient | gradient speedup | stanli source-to-CSV | CmdStan build | CmdStan build + run | approx. first-run speedup |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `gpcm_latent_reg_irt` | 121.762 us | 1.338 ms | 10.99x | 9.58 s | 5.0 s | 166.9 s | ~17x |
| `grsm_latent_reg_irt` | 70.621 us | 762.133 us | 10.79x | 4.97 s | 4.9 s | 71.8 s | ~14x |
| `dogs` | 7.196 us | 63.747 us | 8.86x | 0.53 s | 3.5 s | 5.9 s | ~11x |
| `arK` | 1.749 us | 12.459 us | 7.12x | 0.15 s | 2.8 s | 3.8 s | ~25x |
| `radon_pooled` | 45.888 us | 320.938 us | 6.99x | 0.58 s | 2.6 s | 6.4 s | ~11x |
| `logmesquite_logvash` | 432 ns | 2.841 us | 6.58x | 0.11 s | 3.2 s | 3.6 s | ~33x |
| `logmesquite_logvas` | 480 ns | 3.130 us | 6.52x | 0.11 s | 3.2 s | 3.6 s | ~33x |
| `mesquite` | 479 ns | 3.055 us | 6.38x | 0.16 s | 2.9 s | 3.5 s | ~22x |
| `logmesquite_logva` | 339 ns | 2.070 us | 6.11x | 0.06 s | 3.0 s | 3.3 s | ~55x |
| `logmesquite` | 482 ns | 2.902 us | 6.02x | 0.07 s | 3.0 s | 3.3 s | ~48x |
| `rats_model` | 1.098 us | 6.475 us | 5.90x | 0.16 s | 2.9 s | 3.4 s | ~21x |
| `radon_hierarchical_intercept_noncentered` | 98.000 us | 570.300 us | 5.82x | 10.66 s | 3.3 s | 59.3 s | ~5.6x |
| `radon_hierarchical_intercept_centered` | 97.876 us | 569.143 us | 5.81x | 8.39 s | 3.2 s | 47.9 s | ~5.7x |
| `radon_county_intercept` | 82.192 us | 431.614 us | 5.25x | 5.43 s | 2.9 s | 30.3 s | ~5.6x |
| `radon_variable_intercept_noncentered` | 82.571 us | 430.721 us | 5.22x | 7.40 s | 3.1 s | 36.9 s | ~5.0x |
| `radon_variable_intercept_centered` | 82.447 us | 427.262 us | 5.18x | 4.84 s | 2.9 s | 25.9 s | ~5.4x |
| `dogs_log` | 8.004 us | 41.387 us | 5.17x | 0.46 s | 3.2 s | 4.2 s | ~9.2x |
| `logmesquite_logvolume` | 256 ns | 1.304 us | 5.09x | 0.03 s | 2.9 s | 3.1 s | ~100x |
| `radon_variable_slope_centered` | 83.875 us | 420.987 us | 5.02x | 4.96 s | 2.9 s | 26.6 s | ~5.4x |
| `GLM_Poisson_model` | 401 ns | 2.008 us | 5.01x | 0.06 s | 3.2 s | 3.4 s | ~57x |
| `radon_variable_slope_noncentered` | 84.604 us | 422.894 us | 5.00x | 10.75 s | 3.1 s | 55.0 s | ~5.1x |
| `kilpisjarvi` | 330 ns | 1.532 us | 4.64x | 0.86 s | 2.7 s | 4.3 s | ~5.0x |
| `Mt_model` | 4.470 us | 19.984 us | 4.47x | 0.08 s | 3.4 s | 3.9 s | ~48x |
| `Rate_2_model` | 126 ns | 561 ns | 4.45x | 0.02 s | 2.5 s | 2.7 s | ~140x |
| `nes` | 16.205 us | 69.324 us | 4.28x | 1.53 s | 3.1 s | 9.8 s | ~6.4x |
| `kidscore_interaction_c` | 2.506 us | 10.333 us | 4.12x | 0.08 s | 2.9 s | 3.3 s | ~41x |
| `radon_partially_pooled_centered` | 67.104 us | 272.243 us | 4.06x | 3.82 s | 2.9 s | 16.9 s | ~4.4x |
| `radon_partially_pooled_noncentered` | 67.686 us | 273.685 us | 4.04x | 5.28 s | 3.1 s | 23.2 s | ~4.4x |
| `kidscore_interaction_z` | 2.480 us | 10.013 us | 4.04x | 0.10 s | 2.9 s | 3.4 s | ~34x |
| `Mth_model` | 23.535 us | 93.922 us | 3.99x | 2.42 s | 4.2 s | 9.6 s | ~4.0x |
| `kidscore_interaction` | 2.498 us | 9.927 us | 3.97x | 0.53 s | 2.9 s | 4.8 s | ~9.1x |
| `kidscore_mom_work` | 2.509 us | 9.959 us | 3.97x | 0.13 s | 2.8 s | 3.4 s | ~26x |
| `kidscore_interaction_c2` | 2.500 us | 9.901 us | 3.96x | 0.08 s | 2.8 s | 3.2 s | ~40x |
| `sesame_one_pred_a` | 877 ns | 3.440 us | 3.92x | 0.04 s | 2.7 s | 2.9 s | ~73x |
| `state_space_stochastic_level_stochastic_seasonal` | 6.726 us | 26.320 us | 3.91x | 12.02 s | 4.6 s | 44.4 s | ~3.7x |
| `Rate_1_model` | 67 ns | 260 ns | 3.88x | 0.02 s | 2.3 s | 2.4 s | ~120x |
| `radon_variable_intercept_slope_noncentered` | 121.117 us | 441.463 us | 3.64x | 17.53 s | 3.3 s | 61.9 s | ~3.5x |
| `radon_variable_intercept_slope_centered` | 120.204 us | 437.889 us | 3.64x | 9.01 s | 3.1 s | 30.3 s | ~3.4x |
| `seeds_centered_model` | 739 ns | 2.650 us | 3.59x | 0.08 s | 3.7 s | 4.0 s | ~50x |
| `kidscore_momhsiq` | 2.006 us | 7.145 us | 3.56x | 0.25 s | 2.8 s | 3.6 s | ~15x |
| `M0_model` | 4.414 us | 15.595 us | 3.53x | 0.07 s | 2.7 s | 3.1 s | ~44x |
| `logearn_interaction_z` | 7.497 us | 26.484 us | 3.53x | 0.21 s | 3.0 s | 3.8 s | ~18x |
| `election88_full` | 257.308 us | 901.961 us | 3.51x | 121.58 s | 3.9 s | 472.0 s | ~3.9x |
| `logearn_interaction` | 7.528 us | 26.051 us | 3.46x | 2.49 s | 2.8 s | 11.3 s | ~4.5x |
| `GLMM_Poisson_model` | 698 ns | 2.412 us | 3.46x | 0.20 s | 3.7 s | 4.4 s | ~22x |
| `GLMM1_model` | 10.308 us | 35.558 us | 3.45x | 1.06 s | 3.2 s | 4.9 s | ~4.6x |
| `Mtbh_model` | 12.925 us | 42.791 us | 3.31x | 1.04 s | 4.9 s | 7.3 s | ~7.0x |
| `dogs_hierarchical` | 10.341 us | 34.053 us | 3.29x | 0.28 s | 2.7 s | 3.4 s | ~12x |
| `logearn_logheight_male` | 5.696 us | 18.697 us | 3.28x | 4.30 s | 2.8 s | 16.2 s | ~3.8x |
| `seeds_stanified_model` | 716 ns | 2.341 us | 3.27x | 0.08 s | 3.4 s | 3.7 s | ~46x |
| `logearn_height_male` | 5.888 us | 19.147 us | 3.25x | 1.35 s | 2.8 s | 6.6 s | ~4.9x |
| `eight_schools_noncentered` | 233 ns | 745 ns | 3.20x | 0.03 s | 3.2 s | 3.4 s | ~110x |
| `dugongs_model` | 517 ns | 1.653 us | 3.20x | 0.05 s | 3.0 s | 3.2 s | ~65x |
| `Rate_3_model` | 84 ns | 268 ns | 3.19x | 0.02 s | 2.4 s | 2.6 s | ~130x |
| `surgical_model` | 528 ns | 1.684 us | 3.19x | 0.05 s | 3.3 s | 3.5 s | ~71x |
| `kidscore_momiq` | 1.545 us | 4.861 us | 3.15x | 0.14 s | 2.7 s | 3.1 s | ~22x |
| `Rate_5_model` | 84 ns | 262 ns | 3.12x | 0.02 s | 2.4 s | 2.6 s | ~130x |
| `Rate_4_model` | 101 ns | 311 ns | 3.08x | 0.02 s | 2.4 s | 2.6 s | ~130x |
| `blr` | 570 ns | 1.728 us | 3.03x | 0.05 s | 3.1 s | 3.3 s | ~66x |
| `seeds_model` | 729 ns | 2.130 us | 2.92x | 0.08 s | 3.5 s | 3.8 s | ~47x |
| `kidscore_momhs` | 1.543 us | 4.483 us | 2.91x | 0.06 s | 2.7 s | 3.0 s | ~50x |
| `bym2_offset_only` | 40.012 us | 114.620 us | 2.86x | 16.78 s | 4.1 s | 27.5 s | ~1.6x |
| `logistic_regression_rhs` | 40.743 us | 113.106 us | 2.78x | 11.95 s | 4.6 s | 20.8 s | ~1.7x |
| `log10earn_height` | 4.222 us | 11.560 us | 2.74x | 0.93 s | 2.7 s | 4.5 s | ~4.8x |
| `multi_occupancy` | 22.217 us | 58.996 us | 2.66x | 2.45 s | 5.7 s | 13.0 s | ~5.3x |
| `logearn_height` | 4.250 us | 11.162 us | 2.63x | 0.69 s | 2.7 s | 4.4 s | ~6.4x |
| `earn_height` | 4.224 us | 10.866 us | 2.57x | 0.78 s | 2.7 s | 4.6 s | ~5.8x |
| `Mh_model` | 15.185 us | 38.956 us | 2.57x | 1.17 s | 3.2 s | 5.9 s | ~5.0x |
| `pilots` | 737 ns | 1.878 us | 2.55x | 0.66 s | 3.2 s | 4.5 s | ~6.8x |
| `losscurve_sislob` | 1.384 us | 3.450 us | 2.49x | 0.18 s | 4.1 s | 4.4 s | ~24x |
| `lsat_model` | 37.744 us | 91.173 us | 2.42x | 2.59 s | 3.6 s | 8.3 s | ~3.2x |
| `irt_2pl` | 16.410 us | 37.468 us | 2.28x | 1.31 s | 3.9 s | 6.1 s | ~4.6x |
| `soil_incubation` | 26.731 us | 60.871 us | 2.28x | 6.15 s | 3.3 s | 16.1 s | ~2.6x |
| `GLM_Binomial_model` | 835 ns | 1.809 us | 2.17x | 0.06 s | 3.2 s | 3.4 s | ~57x |
| `ldaK2` | 48.212 us | 104.059 us | 2.16x | 1.27 s | 3.6 s | 6.8 s | ~5.3x |
| `normal_mixture` | 41.662 us | 88.239 us | 2.12x | 0.43 s | 2.5 s | 3.6 s | ~8.4x |
| `dogs_nonhierarchical` | 19.300 us | 40.588 us | 2.10x | 1.13 s | 6.8 s | 9.7 s | ~8.5x |
| `low_dim_gauss_mix_collapse` | 46.506 us | 95.373 us | 2.05x | 2.20 s | 3.0 s | 7.5 s | ~3.4x |
| `low_dim_gauss_mix` | 47.972 us | 98.315 us | 2.05x | 0.80 s | 3.0 s | 5.0 s | ~6.2x |
| `lotka_volterra` | 20.843 us | 41.313 us | 1.98x | 2.16 s | 4.1 s | 10.3 s | ~4.8x |
| `iohmm_reg` | 162.746 us | 320.335 us | 1.97x | 111.37 s | 5.7 s | 186.9 s | ~1.7x |
| `normal_mixture_k` | 189.445 us | 357.439 us | 1.89x | 58.80 s | 3.3 s | 104.9 s | ~1.8x |
| `wells_dist` | 21.091 us | 39.202 us | 1.86x | 0.66 s | 2.8 s | 4.2 s | ~6.4x |
| `accel_gp` | 5.590 us | 9.532 us | 1.71x | 11.25 s | 5.3 s | 22.3 s | ~2.0x |
| `hmm_example` | 16.081 us | 27.145 us | 1.69x | 0.50 s | 4.0 s | 5.0 s | ~10x |
| `accel_splines` | 6.380 us | 10.584 us | 1.66x | 14.64 s | 4.3 s | 24.0 s | ~1.6x |
| `2pl_latent_reg_irt` | 83.462 us | 134.556 us | 1.61x | 6.08 s | 5.3 s | 13.2 s | ~2.2x |
| `hmm_gaussian` | 167.428 us | 263.917 us | 1.58x | 253.57 s | 4.6 s | 23.4 s | ~0.09x |
| `covid19imperial_v2` | 237.788 us | 345.937 us | 1.45x | 123.03 s | 6.7 s | 182.7 s | ~1.5x |
| `hier_2pl` | 278.729 us | 397.603 us | 1.43x | 17.36 s | 6.5 s | 33.3 s | ~1.9x |
| `garch11` | 6.996 us | 9.664 us | 1.38x | 0.20 s | 2.8 s | 3.2 s | ~16x |
| `hmm_drive_1` | 109.425 us | 147.829 us | 1.35x | 4.33 s | 4.7 s | 11.6 s | ~2.7x |
| `covid19imperial_v3` | 266.177 us | 342.943 us | 1.29x | 118.43 s | 6.7 s | 182.4 s | ~1.5x |
| `prophet` | 54.283 us | 69.789 us | 1.29x | 92.62 s | 4.7 s | 122.4 s | ~1.3x |
| `arma11` | 4.806 us | 6.158 us | 1.28x | 0.09 s | 2.9 s | 3.2 s | ~35x |
| `hmm_drive_0` | 103.707 us | 132.850 us | 1.28x | 2.99 s | 4.5 s | 8.2 s | ~2.7x |
| `eight_schools_centered` | 248 ns | 314 ns | 1.27x | 0.04 s | 2.9 s | 3.1 s | ~77x |
| `nes_logit_model` | 6.161 us | 7.653 us | 1.24x | 0.15 s | 3.0 s | 3.4 s | ~23x |
| `bones_model` | 42.708 us | 51.501 us | 1.21x | 0.72 s | 3.4 s | 4.7 s | ~6.5x |
| `Mb_model` | 42.139 us | 49.570 us | 1.18x | 0.96 s | 3.2 s | 4.3 s | ~4.5x |
| `Survey_model` | 54.163 us | 61.578 us | 1.14x | 1.12 s | 2.9 s | 4.0 s | ~3.6x |
| `hierarchical_gp` | 42.189 us | 47.565 us | 1.13x | 24.64 s | 8.6 s | 25.8 s | ~1.0x |
| `radon_county` | 73.549 us | 82.076 us | 1.12x | 4.04 s | 3.0 s | 7.5 s | ~1.9x |
| `kronecker_gp` | 196.635 us | 217.990 us | 1.11x | 392.72 s | 8.0 s | 459.1 s | ~1.2x |
| `wells_dae_c_model` | 17.640 us | 19.308 us | 1.09x | 0.39 s | 3.2 s | 3.8 s | ~9.7x |
| `wells_dist100ars_model` | 17.365 us | 18.997 us | 1.09x | 0.40 s | 3.0 s | 3.6 s | ~9.0x |
| `wells_dae_inter_model` | 19.491 us | 21.310 us | 1.09x | 0.31 s | 3.2 s | 3.8 s | ~12x |
| `wells_daae_c_model` | 19.109 us | 20.885 us | 1.09x | 0.50 s | 3.2 s | 3.8 s | ~7.7x |
| `nn_rbm1bJ10` | 170.858 us | 185.731 us | 1.09x | 496.72 s | 5.2 s | 462.0 s | ~0.93x |
| `wells_interaction_model` | 18.821 us | 20.402 us | 1.08x | 0.69 s | 3.1 s | 4.0 s | ~5.9x |
| `wells_dae_model` | 18.802 us | 20.356 us | 1.08x | 0.55 s | 3.1 s | 3.9 s | ~7.0x |
| `wells_dist100_model` | 15.926 us | 17.195 us | 1.08x | 0.25 s | 3.0 s | 3.5 s | ~14x |
| `wells_interaction_c_model` | 18.876 us | 20.272 us | 1.07x | 0.26 s | 3.1 s | 3.6 s | ~14x |
| `one_comp_mm_elim_abs` | 459.356 us | 470.681 us | 1.02x | 9.18 s | 3.3 s | 14.5 s | ~1.6x |
| `diamonds` | 31.050 us | 31.497 us | 1.01x | 51.59 s | 3.4 s | 51.9 s | ~1.0x |
| `gp_regr` | 5.593 us | 4.698 us | 0.84x | 0.09 s | 5.2 s | 5.4 s | ~60x |
| `gp_pois_regr` | 5.279 us | 3.935 us | 0.75x | 1.60 s | 5.4 s | 6.9 s | ~4.3x |

120 models; 119 with both gradients; median per-gradient speedup 2.92x; 117/119
at or above CmdStan. 117 completed first runs; median source-to-CSV speedup
about 6.8x; 115/117 at or above CmdStan including its model build.

The extreme `hmm_gaussian` first-run result is not a useful speed comparison:
every post-warmup draw in CmdStan's retained seed-1 run was divergent, so the
two engines did radically different sampling work. Its controlled gradient
row, 1.58x in stanli's favor, is the meaningful result.

### Runs that did not complete

A missing run time is not a missing gradient. These rows sort below the
complete table, and their measured gradient ratios still stand.

| model | stanli gradient | CmdStan gradient | gradient speedup | what stopped it |
| --- | ---: | ---: | ---: | --- |
| `ldaK5` | 2.386 ms | 5.580 ms | 2.34x | stanli sampling hit the 900 s cap; CmdStan sampling hit the 900 s cap |
| `nn_rbm1bJ100` | 415.660 ms | 434.981 ms | 1.05x | stanli sampling hit the 900 s cap; CmdStan sampling hit the 900 s cap |
| `sir` | - | - | - | stanli's gradient probe threw at the benchmark point; no stanli gradient |

`sir` has no gradient number because the fixed probe point makes its ODE
solution dip to -4.4e-10 and `poisson_lpmf` rejects the negative rate. The
model itself samples successfully. `ldaK5` and `nn_rbm1bJ100` completed both
gradient probes before both sampling commands reached the 900 s cap.

## Benchmark method

Measured 2026-09-03 on an Apple M3 Ultra (macOS arm64) with Apple clang 21,
single-threaded, with both engines built at `-O3` and
`-ffp-contract=off`. The stanli columns are one refreshed 120-model run. The
CmdStan columns carry over from a 2026-08-06 run on the same host because no
stanli change can affect them. CmdStan's one-time `make build` setup was
already complete, so its per-model build column uses the warm precompiled
header path and does not include that earlier setup cost.

For gradients, both engines evaluate the sampling log density (proportional
terms plus Jacobian) at the same deterministic unconstrained point. stanli
runs `tools/bench_grad.cpp`; CmdStan runs `tools/bench_cmdstan_grad.cpp` over
the stanc-generated model. Both loop the same fresh-vars, gradient, and memory
recovery cycle that a leapfrog step performs. The reported cells are warmed
arithmetic means from one timed loop per model.

For complete runs, `stanli_sample_s` in
[`corpus-bench.tsv`](corpus-bench.tsv) measures the entire `stanli_run`
process from Stan source to CSV. CmdStan's build and execution are timed
separately, so the displayed total adds `cmdstan_build_s` and
`cmdstan_sample_s`. The sampler rows are real wall-clock observations, not a
fixed-work microbenchmark; use the gradient rows when comparing engine
throughput independent of a particular NUTS trajectory.

## For developers

The implementation story is intentionally elsewhere. For a conceptual
overview, read [How stanli works](how-it-works.md). For graph re-rolling, lane
partitioning, generated adjoints, tape islands, the compiled ODE right-hand
side, and their targeted A/B measurements, read
[Graph optimizations and performance work](../runtime/src/OPTIMIZATIONS.md).

## Reproducing

```sh
./tools/dev_setup.sh --corpus          # deps, build, posteriordb, CmdStan
cmake -B build-rel -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel -j
python3 harnesses/corpus_bench.py deps/cmdstan deps/posteriordb \
  docs/corpus-bench.tsv --stanli-only --timeout 900
# To remeasure both sides, omit --stanli-only and use a new output TSV.
python3 tools/corpus_table.py docs/corpus-bench.tsv
python3 harnesses/ab_corpus.py deps/posteriordb
```
