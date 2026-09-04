# How much faster is stanli?

Across 119 posteriordb models, stanli evaluates a gradient **2.93x faster
than CmdStan at the median**. It is at least as fast on 117 of the 119 models.
Because stanli does not build a native C++ binary for each model, the first
complete run is typically faster by more than the gradient ratio alone
suggests.

## Eight Schools: 3.2x faster gradients, roughly 100x faster to draws

The non-centered Eight Schools model is a useful first result because it is
small: there is little work over which either engine can hide overhead.

| measurement | stanli | CmdStan | speedup |
| --- | ---: | ---: | ---: |
| one gradient at the same point | 230 ns | 745 ns | **3.24x** |
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
| `gpcm_latent_reg_irt` | 122.568 us | 1.338 ms | 10.91x | 9.53 s | 166.9 s | ~18x |
| `dogs` | 7.316 us | 63.747 us | 8.71x | 0.53 s | 5.9 s | ~11x |
| `radon_pooled` | 45.912 us | 320.938 us | 6.99x | 0.57 s | 6.4 s | ~11x |
| `GLM_Poisson_model` | 394 ns | 2.008 us | 5.10x | 0.06 s | 3.4 s | ~57x |
| `state_space_stochastic_level_stochastic_seasonal` | 6.833 us | 26.320 us | 3.85x | 12.05 s | 44.4 s | ~3.7x |
| `eight_schools_noncentered` | 230 ns | 745 ns | 3.24x | 0.03 s | 3.4 s | ~110x |
| `logistic_regression_rhs` | 40.626 us | 113.106 us | 2.78x | 11.93 s | 20.8 s | ~1.7x |
| `soil_incubation` | 28.774 us | 60.871 us | 2.12x | 6.47 s | 16.1 s | ~2.5x |
| `normal_mixture` | 42.228 us | 88.239 us | 2.09x | 0.43 s | 3.6 s | ~8.4x |
| `lotka_volterra` | 24.253 us | 41.313 us | 1.70x | 2.13 s | 10.3 s | ~4.9x |
| `hmm_example` | 15.941 us | 27.145 us | 1.70x | 0.48 s | 5.0 s | ~10x |
| `garch11` | 6.954 us | 9.664 us | 1.39x | 0.20 s | 3.2 s | ~16x |
| `hierarchical_gp` | 40.074 us | 47.565 us | 1.19x | 24.14 s | 25.8 s | ~1.1x |
| `one_comp_mm_elim_abs` | 456.593 us | 470.681 us | 1.03x | 9.38 s | 14.5 s | ~1.5x |
| `diamonds` | 30.849 us | 31.497 us | 1.02x | 51.44 s | 51.9 s | ~1.0x |
| `gp_regr` | 5.566 us | 4.698 us | 0.84x | 0.09 s | 5.4 s | ~60x |
| `gp_pois_regr` | 5.205 us | 3.935 us | 0.76x | 1.60 s | 6.9 s | ~4.3x |

Across all 117 models that completed a full run in both engines, the median
source-to-CSV speedup is **about 6.7x**, including CmdStan's model build. 115 of 117
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
ran at 0.87-0.90x CmdStan in the 0.10.0 benchmarks; they are now 1.03-2.12x.
All three call the legacy `integrate_ode_bdf`/`integrate_ode_rk45` interface,
and the change coincided with 0.11.0's expanded function coverage inside ODE
right-hand sides (see the changelog). Both engines still use the same Stan
Math integrator.

**Two Gaussian-process models are now the only gradient losses.** `gp_regr`
and `gp_pois_regr`, at 0.76-0.84x CmdStan, are dominated by covariance-matrix
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
| `gpcm_latent_reg_irt` | 122.568 us | 1.338 ms | 10.91x | 9.53 s | 5.0 s | 166.9 s | ~18x |
| `grsm_latent_reg_irt` | 71.347 us | 762.133 us | 10.68x | 4.99 s | 4.9 s | 71.8 s | ~14x |
| `dogs` | 7.316 us | 63.747 us | 8.71x | 0.53 s | 3.5 s | 5.9 s | ~11x |
| `arK` | 1.738 us | 12.459 us | 7.17x | 0.14 s | 2.8 s | 3.8 s | ~27x |
| `radon_pooled` | 45.912 us | 320.938 us | 6.99x | 0.57 s | 2.6 s | 6.4 s | ~11x |
| `logmesquite_logvash` | 433 ns | 2.841 us | 6.56x | 0.10 s | 3.2 s | 3.6 s | ~36x |
| `logmesquite_logvas` | 479 ns | 3.130 us | 6.53x | 0.11 s | 3.2 s | 3.6 s | ~33x |
| `mesquite` | 488 ns | 3.055 us | 6.26x | 0.16 s | 2.9 s | 3.5 s | ~22x |
| `rats_model` | 1.074 us | 6.475 us | 6.03x | 0.16 s | 2.9 s | 3.4 s | ~21x |
| `logmesquite_logva` | 349 ns | 2.070 us | 5.93x | 0.06 s | 3.0 s | 3.3 s | ~55x |
| `logmesquite` | 491 ns | 2.902 us | 5.91x | 0.07 s | 3.0 s | 3.3 s | ~48x |
| `radon_hierarchical_intercept_centered` | 97.744 us | 569.143 us | 5.82x | 8.42 s | 3.2 s | 47.9 s | ~5.7x |
| `radon_hierarchical_intercept_noncentered` | 98.030 us | 570.300 us | 5.82x | 10.65 s | 3.3 s | 59.3 s | ~5.6x |
| `radon_county_intercept` | 82.314 us | 431.614 us | 5.24x | 5.45 s | 2.9 s | 30.3 s | ~5.6x |
| `radon_variable_intercept_noncentered` | 82.688 us | 430.721 us | 5.21x | 7.44 s | 3.1 s | 36.9 s | ~5.0x |
| `radon_variable_intercept_centered` | 82.631 us | 427.262 us | 5.17x | 4.85 s | 2.9 s | 25.9 s | ~5.3x |
| `dogs_log` | 8.024 us | 41.387 us | 5.16x | 0.46 s | 3.2 s | 4.2 s | ~9.2x |
| `GLM_Poisson_model` | 394 ns | 2.008 us | 5.10x | 0.06 s | 3.2 s | 3.4 s | ~57x |
| `radon_variable_slope_centered` | 83.664 us | 420.987 us | 5.03x | 4.97 s | 2.9 s | 26.6 s | ~5.3x |
| `radon_variable_slope_noncentered` | 84.171 us | 422.894 us | 5.02x | 10.76 s | 3.1 s | 55.0 s | ~5.1x |
| `logmesquite_logvolume` | 274 ns | 1.304 us | 4.76x | 0.03 s | 2.9 s | 3.1 s | ~100x |
| `kilpisjarvi` | 332 ns | 1.532 us | 4.61x | 0.85 s | 2.7 s | 4.3 s | ~5.1x |
| `Rate_2_model` | 124 ns | 561 ns | 4.52x | 0.02 s | 2.5 s | 2.7 s | ~140x |
| `Mt_model` | 4.487 us | 19.984 us | 4.45x | 0.08 s | 3.4 s | 3.9 s | ~48x |
| `nes` | 16.198 us | 69.324 us | 4.28x | 1.52 s | 3.1 s | 9.8 s | ~6.4x |
| `radon_partially_pooled_noncentered` | 67.446 us | 273.685 us | 4.06x | 5.27 s | 3.1 s | 23.2 s | ~4.4x |
| `radon_partially_pooled_centered` | 67.102 us | 272.243 us | 4.06x | 3.82 s | 2.9 s | 16.9 s | ~4.4x |
| `kidscore_interaction_c` | 2.557 us | 10.333 us | 4.04x | 0.08 s | 2.9 s | 3.3 s | ~41x |
| `kidscore_interaction_z` | 2.480 us | 10.013 us | 4.04x | 0.10 s | 2.9 s | 3.4 s | ~34x |
| `Mth_model` | 23.469 us | 93.922 us | 4.00x | 2.42 s | 4.2 s | 9.6 s | ~4.0x |
| `sesame_one_pred_a` | 861 ns | 3.440 us | 4.00x | 0.04 s | 2.7 s | 2.9 s | ~73x |
| `kidscore_mom_work` | 2.493 us | 9.959 us | 3.99x | 0.13 s | 2.8 s | 3.4 s | ~26x |
| `kidscore_interaction_c2` | 2.507 us | 9.901 us | 3.95x | 0.08 s | 2.8 s | 3.2 s | ~40x |
| `kidscore_interaction` | 2.526 us | 9.927 us | 3.93x | 0.53 s | 2.9 s | 4.8 s | ~9.1x |
| `Rate_1_model` | 67 ns | 260 ns | 3.88x | 0.02 s | 2.3 s | 2.4 s | ~120x |
| `state_space_stochastic_level_stochastic_seasonal` | 6.833 us | 26.320 us | 3.85x | 12.05 s | 4.6 s | 44.4 s | ~3.7x |
| `radon_variable_intercept_slope_noncentered` | 121.000 us | 441.463 us | 3.65x | 17.57 s | 3.3 s | 61.9 s | ~3.5x |
| `radon_variable_intercept_slope_centered` | 120.126 us | 437.889 us | 3.65x | 9.01 s | 3.1 s | 30.3 s | ~3.4x |
| `seeds_centered_model` | 740 ns | 2.650 us | 3.58x | 0.08 s | 3.7 s | 4.0 s | ~50x |
| `logearn_interaction_z` | 7.407 us | 26.484 us | 3.58x | 0.21 s | 3.0 s | 3.8 s | ~18x |
| `logearn_interaction` | 7.397 us | 26.051 us | 3.52x | 2.45 s | 2.8 s | 11.3 s | ~4.6x |
| `election88_full` | 256.577 us | 901.961 us | 3.52x | 121.77 s | 3.9 s | 472.0 s | ~3.9x |
| `M0_model` | 4.448 us | 15.595 us | 3.51x | 0.07 s | 2.7 s | 3.1 s | ~44x |
| `GLMM_Poisson_model` | 692 ns | 2.412 us | 3.49x | 0.20 s | 3.7 s | 4.4 s | ~22x |
| `kidscore_momhsiq` | 2.060 us | 7.145 us | 3.47x | 0.25 s | 2.8 s | 3.6 s | ~15x |
| `GLMM1_model` | 10.293 us | 35.558 us | 3.45x | 1.06 s | 3.2 s | 4.9 s | ~4.6x |
| `logearn_height_male` | 5.640 us | 19.147 us | 3.39x | 1.34 s | 2.8 s | 6.6 s | ~4.9x |
| `Mtbh_model` | 12.665 us | 42.791 us | 3.38x | 1.04 s | 4.9 s | 7.3 s | ~7.0x |
| `dogs_hierarchical` | 10.224 us | 34.053 us | 3.33x | 0.24 s | 2.7 s | 3.4 s | ~14x |
| `logearn_logheight_male` | 5.688 us | 18.697 us | 3.29x | 4.31 s | 2.8 s | 16.2 s | ~3.8x |
| `blr` | 527 ns | 1.728 us | 3.28x | 0.05 s | 3.1 s | 3.3 s | ~66x |
| `dugongs_model` | 509 ns | 1.653 us | 3.25x | 0.05 s | 3.0 s | 3.2 s | ~65x |
| `eight_schools_noncentered` | 230 ns | 745 ns | 3.24x | 0.03 s | 3.2 s | 3.4 s | ~110x |
| `surgical_model` | 524 ns | 1.684 us | 3.21x | 0.05 s | 3.3 s | 3.5 s | ~71x |
| `seeds_stanified_model` | 744 ns | 2.341 us | 3.15x | 0.08 s | 3.4 s | 3.7 s | ~46x |
| `kidscore_momiq` | 1.546 us | 4.861 us | 3.14x | 0.14 s | 2.7 s | 3.1 s | ~22x |
| `Rate_4_model` | 100 ns | 311 ns | 3.11x | 0.02 s | 2.4 s | 2.6 s | ~130x |
| `Rate_3_model` | 87 ns | 268 ns | 3.08x | 0.02 s | 2.4 s | 2.6 s | ~130x |
| `Rate_5_model` | 89 ns | 262 ns | 2.94x | 0.02 s | 2.4 s | 2.6 s | ~130x |
| `seeds_model` | 727 ns | 2.130 us | 2.93x | 0.08 s | 3.5 s | 3.8 s | ~47x |
| `kidscore_momhs` | 1.553 us | 4.483 us | 2.89x | 0.06 s | 2.7 s | 3.0 s | ~50x |
| `bym2_offset_only` | 39.736 us | 114.620 us | 2.88x | 16.57 s | 4.1 s | 27.5 s | ~1.7x |
| `logistic_regression_rhs` | 40.626 us | 113.106 us | 2.78x | 11.93 s | 4.6 s | 20.8 s | ~1.7x |
| `log10earn_height` | 4.300 us | 11.560 us | 2.69x | 0.92 s | 2.7 s | 4.5 s | ~4.8x |
| `multi_occupancy` | 22.015 us | 58.996 us | 2.68x | 2.44 s | 5.7 s | 13.0 s | ~5.3x |
| `logearn_height` | 4.178 us | 11.162 us | 2.67x | 0.70 s | 2.7 s | 4.4 s | ~6.3x |
| `pilots` | 712 ns | 1.878 us | 2.64x | 0.67 s | 3.2 s | 4.5 s | ~6.7x |
| `Mh_model` | 15.245 us | 38.956 us | 2.56x | 1.17 s | 3.2 s | 5.9 s | ~5.0x |
| `losscurve_sislob` | 1.360 us | 3.450 us | 2.54x | 0.18 s | 4.1 s | 4.4 s | ~24x |
| `earn_height` | 4.384 us | 10.866 us | 2.48x | 0.78 s | 2.7 s | 4.6 s | ~5.8x |
| `lsat_model` | 37.874 us | 91.173 us | 2.41x | 2.58 s | 3.6 s | 8.3 s | ~3.2x |
| `irt_2pl` | 16.354 us | 37.468 us | 2.29x | 1.30 s | 3.9 s | 6.1 s | ~4.7x |
| `ldaK2` | 48.260 us | 104.059 us | 2.16x | 1.27 s | 3.6 s | 6.8 s | ~5.3x |
| `GLM_Binomial_model` | 839 ns | 1.809 us | 2.16x | 0.06 s | 3.2 s | 3.4 s | ~57x |
| `soil_incubation` | 28.774 us | 60.871 us | 2.12x | 6.47 s | 3.3 s | 16.1 s | ~2.5x |
| `dogs_nonhierarchical` | 19.309 us | 40.588 us | 2.10x | 1.13 s | 6.8 s | 9.7 s | ~8.5x |
| `normal_mixture` | 42.228 us | 88.239 us | 2.09x | 0.43 s | 2.5 s | 3.6 s | ~8.4x |
| `low_dim_gauss_mix_collapse` | 46.491 us | 95.373 us | 2.05x | 2.22 s | 3.0 s | 7.5 s | ~3.4x |
| `low_dim_gauss_mix` | 48.657 us | 98.315 us | 2.02x | 0.82 s | 3.0 s | 5.0 s | ~6.1x |
| `normal_mixture_k` | 187.056 us | 357.439 us | 1.91x | 59.77 s | 3.3 s | 104.9 s | ~1.8x |
| `iohmm_reg` | 171.548 us | 320.335 us | 1.87x | 111.59 s | 5.7 s | 186.9 s | ~1.7x |
| `wells_dist` | 21.094 us | 39.202 us | 1.86x | 0.66 s | 2.8 s | 4.2 s | ~6.4x |
| `lotka_volterra` | 24.253 us | 41.313 us | 1.70x | 2.13 s | 4.1 s | 10.3 s | ~4.9x |
| `hmm_example` | 15.941 us | 27.145 us | 1.70x | 0.48 s | 4.0 s | 5.0 s | ~10x |
| `accel_splines` | 6.234 us | 10.584 us | 1.70x | 14.39 s | 4.3 s | 24.0 s | ~1.7x |
| `accel_gp` | 5.653 us | 9.532 us | 1.69x | 11.14 s | 5.3 s | 22.3 s | ~2.0x |
| `2pl_latent_reg_irt` | 83.220 us | 134.556 us | 1.62x | 6.51 s | 5.3 s | 13.2 s | ~2.0x |
| `hmm_gaussian` | 168.507 us | 263.917 us | 1.57x | 246.81 s | 4.6 s | 23.4 s | ~0.09x |
| `covid19imperial_v2` | 234.950 us | 345.937 us | 1.47x | 112.82 s | 6.7 s | 182.7 s | ~1.6x |
| `covid19imperial_v3` | 234.618 us | 342.943 us | 1.46x | 111.75 s | 6.7 s | 182.4 s | ~1.6x |
| `hier_2pl` | 279.580 us | 397.603 us | 1.42x | 17.38 s | 6.5 s | 33.3 s | ~1.9x |
| `arma11` | 4.400 us | 6.158 us | 1.40x | 0.08 s | 2.9 s | 3.2 s | ~40x |
| `garch11` | 6.954 us | 9.664 us | 1.39x | 0.20 s | 2.8 s | 3.2 s | ~16x |
| `hmm_drive_1` | 109.462 us | 147.829 us | 1.35x | 4.24 s | 4.7 s | 11.6 s | ~2.7x |
| `prophet` | 54.410 us | 69.789 us | 1.28x | 92.45 s | 4.7 s | 122.4 s | ~1.3x |
| `hmm_drive_0` | 105.395 us | 132.850 us | 1.26x | 2.95 s | 4.5 s | 8.2 s | ~2.8x |
| `eight_schools_centered` | 250 ns | 314 ns | 1.26x | 0.04 s | 2.9 s | 3.1 s | ~77x |
| `nes_logit_model` | 6.107 us | 7.653 us | 1.25x | 0.15 s | 3.0 s | 3.4 s | ~23x |
| `Mb_model` | 41.127 us | 49.570 us | 1.21x | 0.96 s | 3.2 s | 4.3 s | ~4.5x |
| `bones_model` | 42.756 us | 51.501 us | 1.20x | 0.73 s | 3.4 s | 4.7 s | ~6.5x |
| `hierarchical_gp` | 40.074 us | 47.565 us | 1.19x | 24.14 s | 8.6 s | 25.8 s | ~1.1x |
| `Survey_model` | 53.886 us | 61.578 us | 1.14x | 1.13 s | 2.9 s | 4.0 s | ~3.6x |
| `radon_county` | 73.142 us | 82.076 us | 1.12x | 4.04 s | 3.0 s | 7.5 s | ~1.9x |
| `wells_dist100ars_model` | 17.129 us | 18.997 us | 1.11x | 0.40 s | 3.0 s | 3.6 s | ~9.0x |
| `kronecker_gp` | 197.178 us | 217.990 us | 1.11x | 392.18 s | 8.0 s | 459.1 s | ~1.2x |
| `wells_dae_c_model` | 17.538 us | 19.308 us | 1.10x | 0.38 s | 3.2 s | 3.8 s | ~10.0x |
| `nn_rbm1bJ10` | 169.299 us | 185.731 us | 1.10x | 491.75 s | 5.2 s | 462.0 s | ~0.94x |
| `wells_interaction_model` | 18.600 us | 20.402 us | 1.10x | 0.69 s | 3.1 s | 4.0 s | ~5.9x |
| `wells_dae_inter_model` | 19.501 us | 21.310 us | 1.09x | 0.31 s | 3.2 s | 3.8 s | ~12x |
| `wells_daae_c_model` | 19.212 us | 20.885 us | 1.09x | 0.50 s | 3.2 s | 3.8 s | ~7.7x |
| `wells_dae_model` | 18.740 us | 20.356 us | 1.09x | 0.55 s | 3.1 s | 3.9 s | ~7.0x |
| `wells_interaction_c_model` | 18.717 us | 20.272 us | 1.08x | 0.26 s | 3.1 s | 3.6 s | ~14x |
| `wells_dist100_model` | 15.894 us | 17.195 us | 1.08x | 0.25 s | 3.0 s | 3.5 s | ~14x |
| `one_comp_mm_elim_abs` | 456.593 us | 470.681 us | 1.03x | 9.38 s | 3.3 s | 14.5 s | ~1.5x |
| `diamonds` | 30.849 us | 31.497 us | 1.02x | 51.44 s | 3.4 s | 51.9 s | ~1.0x |
| `gp_regr` | 5.566 us | 4.698 us | 0.84x | 0.09 s | 5.2 s | 5.4 s | ~60x |
| `gp_pois_regr` | 5.205 us | 3.935 us | 0.76x | 1.60 s | 5.4 s | 6.9 s | ~4.3x |

120 models; 119 with both gradients; median per-gradient speedup 2.93x; 117/119
at or above CmdStan. 117 completed first runs; median source-to-CSV speedup
about 6.7x; 115/117 at or above CmdStan including its model build.

The extreme `hmm_gaussian` first-run result is not a useful speed comparison:
every post-warmup draw in CmdStan's retained seed-1 run was divergent, so the
two engines did radically different sampling work. Its controlled gradient
row, 1.57x in stanli's favor, is the meaningful result.

### Runs that did not complete

A missing run time is not a missing gradient. These rows sort below the
complete table, and their measured gradient ratios still stand.

| model | stanli gradient | CmdStan gradient | gradient speedup | what stopped it |
| --- | ---: | ---: | ---: | --- |
| `ldaK5` | 2.357 ms | 5.580 ms | 2.37x | stanli sampling hit the 900 s cap; CmdStan sampling hit the 900 s cap |
| `nn_rbm1bJ100` | 413.099 ms | 434.981 ms | 1.05x | stanli sampling hit the 900 s cap; CmdStan sampling hit the 900 s cap |
| `sir` | - | - | - | stanli's gradient probe threw at the benchmark point; no stanli gradient |

`sir` has no gradient number because the fixed probe point makes its ODE
solution dip to -4.4e-10 and `poisson_lpmf` rejects the negative rate. The
model itself samples successfully. `ldaK5` and `nn_rbm1bJ100` completed both
gradient probes before both sampling commands reached the 900 s cap.

## Benchmark method

Measured 2026-09-04 on an Apple M3 Ultra (macOS arm64) with Apple clang 21,
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
