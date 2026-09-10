# How much faster is stanli?

Across 119 posteriordb models, stanli evaluates a gradient **3.02x faster
than CmdStan at the median**. It is at least as fast on 117 of the 119 models.
Because stanli does not build a native C++ binary for each model, the first
complete run is typically faster by more than the gradient ratio alone
suggests.

## Eight Schools: 3.8x faster gradients, roughly 100x faster to draws

The non-centered Eight Schools model is a useful first result because it is
small: there is little work over which either engine can hide overhead.

| measurement | stanli | CmdStan | speedup |
| --- | ---: | ---: | ---: |
| one gradient at the same point | 195 ns | 745 ns | **3.82x** |
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
| `gpcm_latent_reg_irt` | 122.226 us | 1.338 ms | 10.94x | 9.30 s | 166.9 s | ~18x |
| `dogs` | 7.416 us | 63.747 us | 8.60x | 0.53 s | 5.9 s | ~11x |
| `radon_pooled` | 43.593 us | 320.938 us | 7.36x | 0.51 s | 6.4 s | ~13x |
| `GLM_Poisson_model` | 355 ns | 2.008 us | 5.66x | 0.06 s | 3.4 s | ~57x |
| `state_space_stochastic_level_stochastic_seasonal` | 6.231 us | 26.320 us | 4.22x | 10.01 s | 44.4 s | ~4.4x |
| `eight_schools_noncentered` | 195 ns | 745 ns | 3.82x | 0.03 s | 3.4 s | ~110x |
| `logistic_regression_rhs` | 40.276 us | 113.106 us | 2.81x | 9.98 s | 20.8 s | ~2.1x |
| `soil_incubation` | 27.902 us | 60.871 us | 2.18x | 6.18 s | 16.1 s | ~2.6x |
| `normal_mixture` | 42.378 us | 88.239 us | 2.08x | 0.44 s | 3.6 s | ~8.2x |
| `lotka_volterra` | 21.291 us | 41.313 us | 1.94x | 2.15 s | 10.3 s | ~4.8x |
| `hmm_example` | 15.458 us | 27.145 us | 1.76x | 0.49 s | 5.0 s | ~10x |
| `garch11` | 6.899 us | 9.664 us | 1.40x | 0.20 s | 3.2 s | ~16x |
| `hierarchical_gp` | 36.665 us | 47.565 us | 1.30x | 15.92 s | 25.8 s | ~1.6x |
| `one_comp_mm_elim_abs` | 463.692 us | 470.681 us | 1.02x | 9.35 s | 14.5 s | ~1.6x |
| `diamonds` | 31.034 us | 31.497 us | 1.01x | 49.95 s | 51.9 s | ~1.0x |
| `gp_regr` | 5.603 us | 4.698 us | 0.84x | 0.09 s | 5.4 s | ~60x |
| `gp_pois_regr` | 5.238 us | 3.935 us | 0.75x | 1.58 s | 6.9 s | ~4.3x |

Across all 117 models that completed a full run in both engines, the median
source-to-CSV speedup is **about 8.5x**, including CmdStan's model build. 116 of 117
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
ran at 0.87-0.90x CmdStan in the 0.10.0 benchmarks; they are now 1.02-2.18x.
All three call the legacy `integrate_ode_bdf`/`integrate_ode_rk45` interface,
and the change coincided with 0.11.0's expanded function coverage inside ODE
right-hand sides (see the changelog). Both engines still use the same Stan
Math integrator.

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
| `grsm_latent_reg_irt` | 69.589 us | 762.133 us | 10.95x | 4.86 s | 4.9 s | 71.8 s | ~15x |
| `gpcm_latent_reg_irt` | 122.226 us | 1.338 ms | 10.94x | 9.30 s | 5.0 s | 166.9 s | ~18x |
| `radon_hierarchical_intercept_centered` | 65.504 us | 569.143 us | 8.69x | 5.68 s | 3.2 s | 47.9 s | ~8.4x |
| `radon_county_intercept` | 49.718 us | 431.614 us | 8.68x | 3.56 s | 2.9 s | 30.3 s | ~8.5x |
| `radon_hierarchical_intercept_noncentered` | 65.775 us | 570.300 us | 8.67x | 7.57 s | 3.3 s | 59.3 s | ~7.8x |
| `dogs` | 7.416 us | 63.747 us | 8.60x | 0.53 s | 3.5 s | 5.9 s | ~11x |
| `radon_variable_intercept_noncentered` | 50.240 us | 430.721 us | 8.57x | 4.61 s | 3.1 s | 36.9 s | ~8.0x |
| `radon_variable_intercept_centered` | 50.052 us | 427.262 us | 8.54x | 3.01 s | 2.9 s | 25.9 s | ~8.6x |
| `radon_variable_slope_centered` | 51.442 us | 420.987 us | 8.18x | 3.12 s | 2.9 s | 26.6 s | ~8.5x |
| `radon_variable_slope_noncentered` | 51.880 us | 422.894 us | 8.15x | 6.59 s | 3.1 s | 55.0 s | ~8.4x |
| `radon_partially_pooled_centered` | 34.644 us | 272.243 us | 7.86x | 2.08 s | 2.9 s | 16.9 s | ~8.1x |
| `radon_partially_pooled_noncentered` | 35.128 us | 273.685 us | 7.79x | 2.79 s | 3.1 s | 23.2 s | ~8.3x |
| `radon_variable_intercept_slope_centered` | 57.953 us | 437.889 us | 7.56x | 4.59 s | 3.1 s | 30.3 s | ~6.6x |
| `arK` | 1.659 us | 12.459 us | 7.51x | 0.13 s | 2.8 s | 3.8 s | ~29x |
| `radon_variable_intercept_slope_noncentered` | 58.843 us | 441.463 us | 7.50x | 8.50 s | 3.3 s | 61.9 s | ~7.3x |
| `radon_pooled` | 43.593 us | 320.938 us | 7.36x | 0.51 s | 2.6 s | 6.4 s | ~13x |
| `logmesquite_logvash` | 420 ns | 2.841 us | 6.76x | 0.09 s | 3.2 s | 3.6 s | ~40x |
| `logmesquite_logvas` | 464 ns | 3.130 us | 6.75x | 0.09 s | 3.2 s | 3.6 s | ~40x |
| `mesquite` | 482 ns | 3.055 us | 6.34x | 0.14 s | 2.9 s | 3.5 s | ~25x |
| `logmesquite_logva` | 328 ns | 2.070 us | 6.31x | 0.06 s | 3.0 s | 3.3 s | ~55x |
| `logmesquite` | 464 ns | 2.902 us | 6.25x | 0.06 s | 3.0 s | 3.3 s | ~56x |
| `rats_model` | 1.054 us | 6.475 us | 6.14x | 0.14 s | 2.9 s | 3.4 s | ~24x |
| `GLM_Poisson_model` | 355 ns | 2.008 us | 5.66x | 0.06 s | 3.2 s | 3.4 s | ~57x |
| `logmesquite_logvolume` | 243 ns | 1.304 us | 5.37x | 0.03 s | 2.9 s | 3.1 s | ~100x |
| `dogs_log` | 8.059 us | 41.387 us | 5.14x | 0.47 s | 3.2 s | 4.2 s | ~9.0x |
| `kilpisjarvi` | 306 ns | 1.532 us | 5.01x | 0.72 s | 2.7 s | 4.3 s | ~6.0x |
| `Mt_model` | 4.487 us | 19.984 us | 4.45x | 0.08 s | 3.4 s | 3.9 s | ~48x |
| `Rate_2_model` | 127 ns | 561 ns | 4.42x | 0.02 s | 2.5 s | 2.7 s | ~140x |
| `nes` | 15.986 us | 69.324 us | 4.34x | 1.49 s | 3.1 s | 9.8 s | ~6.6x |
| `sesame_one_pred_a` | 808 ns | 3.440 us | 4.26x | 0.03 s | 2.7 s | 2.9 s | ~98x |
| `state_space_stochastic_level_stochastic_seasonal` | 6.231 us | 26.320 us | 4.22x | 10.01 s | 4.6 s | 44.4 s | ~4.4x |
| `kidscore_interaction_c` | 2.455 us | 10.333 us | 4.21x | 0.07 s | 2.9 s | 3.3 s | ~47x |
| `kidscore_interaction_z` | 2.434 us | 10.013 us | 4.11x | 0.10 s | 2.9 s | 3.4 s | ~34x |
| `kidscore_mom_work` | 2.445 us | 9.959 us | 4.07x | 0.12 s | 2.8 s | 3.4 s | ~29x |
| `kidscore_interaction_c2` | 2.443 us | 9.901 us | 4.05x | 0.08 s | 2.8 s | 3.2 s | ~40x |
| `kidscore_interaction` | 2.460 us | 9.927 us | 4.04x | 0.50 s | 2.9 s | 4.8 s | ~9.6x |
| `election88_full` | 224.489 us | 901.961 us | 4.02x | 105.00 s | 3.9 s | 472.0 s | ~4.5x |
| `Mth_model` | 23.669 us | 93.922 us | 3.97x | 2.35 s | 4.2 s | 9.6 s | ~4.1x |
| `eight_schools_noncentered` | 195 ns | 745 ns | 3.82x | 0.03 s | 3.2 s | 3.4 s | ~110x |
| `GLMM_Poisson_model` | 636 ns | 2.412 us | 3.79x | 0.17 s | 3.7 s | 4.4 s | ~26x |
| `seeds_centered_model` | 702 ns | 2.650 us | 3.77x | 0.07 s | 3.7 s | 4.0 s | ~57x |
| `Rate_1_model` | 69 ns | 260 ns | 3.77x | 0.01 s | 2.3 s | 2.4 s | ~240x |
| `logearn_interaction` | 7.146 us | 26.051 us | 3.65x | 2.55 s | 2.8 s | 11.3 s | ~4.4x |
| `logearn_interaction_z` | 7.290 us | 26.484 us | 3.63x | 0.22 s | 3.0 s | 3.8 s | ~17x |
| `GLMM1_model` | 9.817 us | 35.558 us | 3.62x | 1.01 s | 3.2 s | 4.9 s | ~4.8x |
| `kidscore_momhsiq` | 1.982 us | 7.145 us | 3.60x | 0.23 s | 2.8 s | 3.6 s | ~16x |
| `M0_model` | 4.448 us | 15.595 us | 3.51x | 0.07 s | 2.7 s | 3.1 s | ~44x |
| `logearn_height_male` | 5.490 us | 19.147 us | 3.49x | 1.20 s | 2.8 s | 6.6 s | ~5.5x |
| `blr` | 503 ns | 1.728 us | 3.44x | 0.04 s | 3.1 s | 3.3 s | ~83x |
| `logearn_logheight_male` | 5.468 us | 18.697 us | 3.42x | 4.17 s | 2.8 s | 16.2 s | ~3.9x |
| `surgical_model` | 497 ns | 1.684 us | 3.39x | 0.04 s | 3.3 s | 3.5 s | ~88x |
| `seeds_stanified_model` | 700 ns | 2.341 us | 3.34x | 0.07 s | 3.4 s | 3.7 s | ~53x |
| `Mtbh_model` | 12.966 us | 42.791 us | 3.30x | 1.02 s | 4.9 s | 7.3 s | ~7.1x |
| `bym2_offset_only` | 35.567 us | 114.620 us | 3.22x | 12.81 s | 4.1 s | 27.5 s | ~2.1x |
| `kidscore_momiq` | 1.511 us | 4.861 us | 3.22x | 0.13 s | 2.7 s | 3.1 s | ~24x |
| `seeds_model` | 680 ns | 2.130 us | 3.13x | 0.08 s | 3.5 s | 3.8 s | ~47x |
| `dugongs_model` | 528 ns | 1.653 us | 3.13x | 0.05 s | 3.0 s | 3.2 s | ~65x |
| `Rate_5_model` | 86 ns | 262 ns | 3.05x | 0.02 s | 2.4 s | 2.6 s | ~130x |
| `Rate_3_model` | 88 ns | 268 ns | 3.05x | 0.01 s | 2.4 s | 2.6 s | ~260x |
| `pilots` | 622 ns | 1.878 us | 3.02x | 0.32 s | 3.2 s | 4.5 s | ~14x |
| `losscurve_sislob` | 1.147 us | 3.450 us | 3.01x | 0.16 s | 4.1 s | 4.4 s | ~28x |
| `Rate_4_model` | 104 ns | 311 ns | 2.99x | 0.02 s | 2.4 s | 2.6 s | ~130x |
| `kidscore_momhs` | 1.507 us | 4.483 us | 2.97x | 0.06 s | 2.7 s | 3.0 s | ~50x |
| `log10earn_height` | 4.055 us | 11.560 us | 2.85x | 0.86 s | 2.7 s | 4.5 s | ~5.2x |
| `logistic_regression_rhs` | 40.276 us | 113.106 us | 2.81x | 9.98 s | 4.6 s | 20.8 s | ~2.1x |
| `logearn_height` | 4.026 us | 11.162 us | 2.77x | 0.65 s | 2.7 s | 4.4 s | ~6.8x |
| `earn_height` | 4.055 us | 10.866 us | 2.68x | 0.71 s | 2.7 s | 4.6 s | ~6.4x |
| `dogs_nonhierarchical` | 15.359 us | 40.588 us | 2.64x | 0.92 s | 6.8 s | 9.7 s | ~10x |
| `Mh_model` | 14.853 us | 38.956 us | 2.62x | 1.10 s | 3.2 s | 5.9 s | ~5.3x |
| `multi_occupancy` | 22.719 us | 58.996 us | 2.60x | 2.88 s | 5.7 s | 13.0 s | ~4.5x |
| `dogs_hierarchical` | 13.692 us | 34.053 us | 2.49x | 0.24 s | 2.7 s | 3.4 s | ~14x |
| `lsat_model` | 37.480 us | 91.173 us | 2.43x | 2.42 s | 3.6 s | 8.3 s | ~3.4x |
| `accel_splines` | 4.632 us | 10.584 us | 2.28x | 10.63 s | 4.3 s | 24.0 s | ~2.3x |
| `irt_2pl` | 16.444 us | 37.468 us | 2.28x | 1.27 s | 3.9 s | 6.1 s | ~4.8x |
| `GLM_Binomial_model` | 812 ns | 1.809 us | 2.23x | 0.05 s | 3.2 s | 3.4 s | ~68x |
| `soil_incubation` | 27.902 us | 60.871 us | 2.18x | 6.18 s | 3.3 s | 16.1 s | ~2.6x |
| `ldaK2` | 47.968 us | 104.059 us | 2.17x | 1.29 s | 3.6 s | 6.8 s | ~5.3x |
| `accel_gp` | 4.430 us | 9.532 us | 2.15x | 8.71 s | 5.3 s | 22.3 s | ~2.6x |
| `2pl_latent_reg_irt` | 63.897 us | 134.556 us | 2.11x | 5.40 s | 5.3 s | 13.2 s | ~2.5x |
| `normal_mixture` | 42.378 us | 88.239 us | 2.08x | 0.44 s | 2.5 s | 3.6 s | ~8.2x |
| `low_dim_gauss_mix_collapse` | 46.023 us | 95.373 us | 2.07x | 2.20 s | 3.0 s | 7.5 s | ~3.4x |
| `hier_2pl` | 192.664 us | 397.603 us | 2.06x | 12.46 s | 6.5 s | 33.3 s | ~2.7x |
| `low_dim_gauss_mix` | 47.949 us | 98.315 us | 2.05x | 0.80 s | 3.0 s | 5.0 s | ~6.2x |
| `radon_county` | 41.018 us | 82.076 us | 2.00x | 2.02 s | 3.0 s | 7.5 s | ~3.7x |
| `prophet` | 34.964 us | 69.789 us | 2.00x | 59.65 s | 4.7 s | 122.4 s | ~2.1x |
| `normal_mixture_k` | 180.127 us | 357.439 us | 1.98x | 61.29 s | 3.3 s | 104.9 s | ~1.7x |
| `iohmm_reg` | 162.271 us | 320.335 us | 1.97x | 153.96 s | 5.7 s | 186.9 s | ~1.2x |
| `lotka_volterra` | 21.291 us | 41.313 us | 1.94x | 2.15 s | 4.1 s | 10.3 s | ~4.8x |
| `wells_dist` | 21.133 us | 39.202 us | 1.86x | 0.66 s | 2.8 s | 4.2 s | ~6.4x |
| `hmm_example` | 15.458 us | 27.145 us | 1.76x | 0.49 s | 4.0 s | 5.0 s | ~10x |
| `hmm_gaussian` | 164.538 us | 263.917 us | 1.60x | 322.50 s | 4.6 s | 23.4 s | ~0.07x |
| `covid19imperial_v2` | 230.798 us | 345.937 us | 1.50x | 120.45 s | 6.7 s | 182.7 s | ~1.5x |
| `covid19imperial_v3` | 230.350 us | 342.943 us | 1.49x | 120.52 s | 6.7 s | 182.4 s | ~1.5x |
| `arma11` | 4.228 us | 6.158 us | 1.46x | 0.08 s | 2.9 s | 3.2 s | ~40x |
| `eight_schools_centered` | 220 ns | 314 ns | 1.43x | 0.04 s | 2.9 s | 3.1 s | ~77x |
| `garch11` | 6.899 us | 9.664 us | 1.40x | 0.20 s | 2.8 s | 3.2 s | ~16x |
| `hmm_drive_1` | 109.211 us | 147.829 us | 1.35x | 4.32 s | 4.7 s | 11.6 s | ~2.7x |
| `hierarchical_gp` | 36.665 us | 47.565 us | 1.30x | 15.92 s | 8.6 s | 25.8 s | ~1.6x |
| `bones_model` | 40.328 us | 51.501 us | 1.28x | 0.71 s | 3.4 s | 4.7 s | ~6.6x |
| `hmm_drive_0` | 105.456 us | 132.850 us | 1.26x | 3.00 s | 4.5 s | 8.2 s | ~2.7x |
| `nes_logit_model` | 6.121 us | 7.653 us | 1.25x | 0.15 s | 3.0 s | 3.4 s | ~23x |
| `Mb_model` | 41.593 us | 49.570 us | 1.19x | 0.96 s | 3.2 s | 4.3 s | ~4.5x |
| `Survey_model` | 53.878 us | 61.578 us | 1.14x | 1.12 s | 2.9 s | 4.0 s | ~3.6x |
| `kronecker_gp` | 193.464 us | 217.990 us | 1.13x | 389.84 s | 8.0 s | 459.1 s | ~1.2x |
| `wells_dist100_model` | 15.516 us | 17.195 us | 1.11x | 0.25 s | 3.0 s | 3.5 s | ~14x |
| `nn_rbm1bJ10` | 167.995 us | 185.731 us | 1.11x | 431.64 s | 5.2 s | 462.0 s | ~1.1x |
| `wells_dist100ars_model` | 17.211 us | 18.997 us | 1.10x | 0.40 s | 3.0 s | 3.6 s | ~9.0x |
| `wells_dae_inter_model` | 19.464 us | 21.310 us | 1.09x | 0.32 s | 3.2 s | 3.8 s | ~12x |
| `wells_daae_c_model` | 19.127 us | 20.885 us | 1.09x | 0.42 s | 3.2 s | 3.8 s | ~9.1x |
| `wells_dae_model` | 18.683 us | 20.356 us | 1.09x | 0.55 s | 3.1 s | 3.9 s | ~7.0x |
| `wells_dae_c_model` | 17.860 us | 19.308 us | 1.08x | 0.35 s | 3.2 s | 3.8 s | ~11x |
| `wells_interaction_model` | 18.909 us | 20.402 us | 1.08x | 0.69 s | 3.1 s | 4.0 s | ~5.9x |
| `wells_interaction_c_model` | 18.799 us | 20.272 us | 1.08x | 0.26 s | 3.1 s | 3.6 s | ~14x |
| `one_comp_mm_elim_abs` | 463.692 us | 470.681 us | 1.02x | 9.35 s | 3.3 s | 14.5 s | ~1.6x |
| `diamonds` | 31.034 us | 31.497 us | 1.01x | 49.95 s | 3.4 s | 51.9 s | ~1.0x |
| `gp_regr` | 5.603 us | 4.698 us | 0.84x | 0.09 s | 5.2 s | 5.4 s | ~60x |
| `gp_pois_regr` | 5.238 us | 3.935 us | 0.75x | 1.58 s | 5.4 s | 6.9 s | ~4.3x |

120 models; 119 with both gradients; median per-gradient speedup 3.02x; 117/119
at or above CmdStan. 117 completed first runs; median source-to-CSV speedup
about 8.5x; 116/117 at or above CmdStan including its model build.

The extreme `hmm_gaussian` first-run result is not a useful speed comparison:
every post-warmup draw in CmdStan's retained seed-1 run was divergent, so the
two engines did radically different sampling work. Its controlled gradient
row, 1.60x in stanli's favor, is the meaningful result.

### Runs that did not complete

A missing run time is not a missing gradient. These rows sort below the
complete table, and their measured gradient ratios still stand.

| model | stanli gradient | CmdStan gradient | gradient speedup | what stopped it |
| --- | ---: | ---: | ---: | --- |
| `ldaK5` | 2.279 ms | 5.580 ms | 2.45x | stanli sampling hit the 900 s cap; CmdStan sampling hit the 900 s cap |
| `nn_rbm1bJ100` | 413.500 ms | 434.981 ms | 1.05x | stanli sampling hit the 900 s cap; CmdStan sampling hit the 900 s cap |
| `sir` | - | - | - | stanli's gradient probe threw at the benchmark point; no stanli gradient |

`sir` has no gradient number because the fixed probe point makes its ODE
solution dip to -4.4e-10 and `poisson_lpmf` rejects the negative rate. The
model itself samples successfully. `ldaK5` and `nn_rbm1bJ100` completed both
gradient probes before both sampling commands reached the 900 s cap.

## Against CmdStan with stanc3's optimizer and loop vectorizer

The tables above build CmdStan the way most users build it, with `stanc` and
no extra flags. This section instead builds CmdStan with `stanc3` at the
pinned commit `8e154ac34`, patched so `--O1` also runs `vectorize_loops`, the
same pass stanli's own pipeline runs by default (see
[`compiler/ocaml/stanli_pipeline.ml`](../compiler/ocaml/stanli_pipeline.ml)).
It uses the same host and driver as the main table above. Only gradient and
compile time are measured; there is no sampling column.

The 2000-gradient column estimates the cost of a short run: compile time plus
two thousand gradient evaluations. For most models CmdStan's per-model build
dominates that cost, so the column tracks close to the compile-time ratio and
approximates the cost of a first short run rather than sampler throughput.

CmdStan's build time here uses the same warm precompiled-header path as the
main table.

<!-- corpus_table.py docs/corpus-bench-o1vec.tsv --o1vec -->
| model | params | stanli gradient | CmdStan gradient | gradient speedup | stanli compile | CmdStan compile | 2000-gradient speedup |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `gpcm_latent_reg_irt` | 530 | 121.634 us | 1.377 ms | 11.32x | 0.088 s | 5.3 s | ~24x |
| `grsm_latent_reg_irt` | 408 | 69.763 us | 741.138 us | 10.62x | 0.029 s | 5.1 s | ~39x |
| `radon_partially_pooled_centered` | 389 | 34.449 us | 270.227 us | 7.84x | 0.028 s | 2.9 s | ~35x |
| `radon_partially_pooled_noncentered` | 389 | 34.993 us | 272.507 us | 7.79x | 0.029 s | 3.1 s | ~37x |
| `radon_hierarchical_intercept_centered` | 391 | 65.539 us | 451.435 us | 6.89x | 0.057 s | 3.1 s | ~21x |
| `radon_hierarchical_intercept_noncentered` | 391 | 65.963 us | 453.354 us | 6.87x | 0.056 s | 3.2 s | ~22x |
| `radon_variable_intercept_noncentered` | 390 | 50.210 us | 343.479 us | 6.84x | 0.035 s | 3.1 s | ~28x |
| `radon_county_intercept` | 388 | 49.692 us | 339.590 us | 6.83x | 0.035 s | 2.8 s | ~26x |
| `radon_variable_intercept_centered` | 390 | 49.669 us | 338.730 us | 6.82x | 0.036 s | 2.9 s | ~27x |
| `radon_variable_slope_centered` | 390 | 51.389 us | 341.040 us | 6.64x | 0.035 s | 2.9 s | ~26x |
| `radon_variable_slope_noncentered` | 390 | 51.811 us | 343.768 us | 6.64x | 0.036 s | 3.1 s | ~27x |
| `radon_variable_intercept_slope_centered` | 777 | 57.578 us | 352.938 us | 6.13x | 0.040 s | 3.0 s | ~24x |
| `radon_variable_intercept_slope_noncentered` | 777 | 58.477 us | 358.167 us | 6.12x | 0.040 s | 3.3 s | ~26x |
| `arK` | 7 | 1.678 us | 8.141 us | 4.85x | 0.002 s | 2.7 s | ~500x |
| `dogs` | 3 | 6.206 us | 27.642 us | 4.45x | 0.014 s | 4.0 s | ~150x |
| `dogs_log` | 2 | 6.094 us | 27.105 us | 4.45x | 0.014 s | 3.6 s | ~140x |
| `Mth_model` | 394 | 23.522 us | 101.609 us | 4.32x | 0.017 s | 4.1 s | ~67x |
| `rats_model` | 65 | 1.058 us | 4.368 us | 4.13x | 0.001 s | 2.9 s | ~950x |
| `Mt_model` | 4 | 4.469 us | 17.407 us | 3.90x | 0.002 s | 3.3 s | ~310x |
| `M0_model` | 2 | 4.435 us | 14.715 us | 3.32x | 0.002 s | 2.6 s | ~250x |
| `GLMM1_model` | 237 | 9.750 us | 31.084 us | 3.19x | 0.004 s | 3.1 s | ~140x |
| `Mtbh_model` | 154 | 12.769 us | 39.106 us | 3.06x | 0.017 s | 4.7 s | ~110x |
| `state_space_stochastic_level_stochastic_seasonal` | 389 | 6.137 us | 18.357 us | 2.99x | 0.002 s | 4.6 s | ~320x |
| `mesquite` | 8 | 467 ns | 1.275 us | 2.73x | 0.000 s | 3.2 s | ~2420x |
| `logmesquite_logvas` | 8 | 467 ns | 1.263 us | 2.70x | 0.000 s | 3.5 s | ~2530x |
| `logmesquite` | 8 | 473 ns | 1.275 us | 2.70x | 0.000 s | 3.2 s | ~2340x |
| `nes` | 10 | 15.929 us | 42.199 us | 2.65x | 0.003 s | 3.4 s | ~99x |
| `logmesquite_logvash` | 7 | 422 ns | 1.102 us | 2.61x | 0.000 s | 3.4 s | ~2720x |
| `dogs_nonhierarchical` | 65 | 15.308 us | 39.686 us | 2.59x | 0.013 s | 6.5 s | ~150x |
| `ldaK5` | 7714 | 2.281 ms | 5.517 ms | 2.42x | 0.439 s | 3.5 s | ~2.9x |
| `dogs_hierarchical` | 2 | 13.684 us | 32.890 us | 2.40x | 0.012 s | 2.6 s | ~68x |
| `kidscore_interaction_c2` | 5 | 2.463 us | 5.731 us | 2.33x | 0.000 s | 3.0 s | ~560x |
| `multi_occupancy` | 106 | 22.884 us | 53.070 us | 2.32x | 0.008 s | 6.1 s | ~110x |
| `kidscore_interaction` | 5 | 2.462 us | 5.705 us | 2.32x | 0.000 s | 3.0 s | ~560x |
| `kidscore_interaction_c` | 5 | 2.442 us | 5.655 us | 2.32x | 0.000 s | 3.0 s | ~560x |
| `kidscore_mom_work` | 5 | 2.474 us | 5.720 us | 2.31x | 0.001 s | 3.0 s | ~480x |
| `kidscore_interaction_z` | 5 | 2.449 us | 5.621 us | 2.30x | 0.000 s | 3.1 s | ~580x |
| `logmesquite_logva` | 5 | 331 ns | 758 ns | 2.29x | 0.000 s | 3.2 s | ~3110x |
| `logearn_interaction_z` | 5 | 7.023 us | 16.005 us | 2.28x | 0.001 s | 3.1 s | ~210x |
| `logearn_interaction` | 5 | 7.116 us | 15.949 us | 2.24x | 0.001 s | 3.0 s | ~200x |
| `Mh_model` | 388 | 15.137 us | 33.141 us | 2.19x | 0.006 s | 3.1 s | ~87x |
| `radon_county` | 389 | 34.522 us | 73.725 us | 2.14x | 0.006 s | 3.0 s | ~42x |
| `logearn_height_male` | 4 | 5.489 us | 11.713 us | 2.13x | 0.001 s | 2.9 s | ~250x |
| `logearn_logheight_male` | 4 | 5.580 us | 11.742 us | 2.10x | 0.001 s | 2.9 s | ~250x |
| `low_dim_gauss_mix` | 5 | 48.118 us | 100.962 us | 2.10x | 0.004 s | 3.1 s | ~33x |
| `normal_mixture` | 3 | 42.440 us | 88.345 us | 2.08x | 0.003 s | 2.7 s | ~33x |
| `kidscore_momhsiq` | 4 | 2.003 us | 4.121 us | 2.06x | 0.000 s | 2.9 s | ~660x |
| `low_dim_gauss_mix_collapse` | 5 | 46.288 us | 94.630 us | 2.04x | 0.004 s | 3.0 s | ~33x |
| `GLM_Poisson_model` | 4 | 358 ns | 729 ns | 2.04x | 0.000 s | 3.1 s | ~2830x |
| `election88_full` | 90 | 215.937 us | 437.064 us | 2.02x | 0.007 s | 4.8 s | ~13x |
| `ldaK2` | 7 | 48.158 us | 95.585 us | 1.98x | 0.008 s | 3.5 s | ~35x |
| `soil_incubation` | 6 | 28.844 us | 55.209 us | 1.91x | 0.001 s | 3.7 s | ~65x |
| `GLMM_Poisson_model` | 45 | 640 ns | 1.205 us | 1.88x | 0.000 s | 3.6 s | ~2080x |
| `lotka_volterra` | 8 | 21.183 us | 39.783 us | 1.88x | 0.001 s | 4.3 s | ~100x |
| `normal_mixture_k` | 14 | 191.635 us | 357.820 us | 1.87x | 0.033 s | 3.3 s | ~9.6x |
| `seeds_centered_model` | 26 | 699 ns | 1.301 us | 1.86x | 0.000 s | 3.7 s | ~2070x |
| `dugongs_model` | 4 | 503 ns | 933 ns | 1.85x | 0.000 s | 2.9 s | ~2040x |
| `iohmm_reg` | 29 | 166.636 us | 305.087 us | 1.83x | 0.478 s | 5.9 s | ~8.0x |
| `radon_pooled` | 3 | 43.770 us | 79.097 us | 1.81x | 0.006 s | 2.7 s | ~31x |
| `losscurve_sislob` | 15 | 1.048 us | 1.874 us | 1.79x | 0.002 s | 4.4 s | ~980x |
| `log10earn_height` | 3 | 4.039 us | 7.218 us | 1.79x | 0.001 s | 2.8 s | ~330x |
| `sesame_one_pred_a` | 3 | 818 ns | 1.442 us | 1.76x | 0.000 s | 2.7 s | ~1380x |
| `earn_height` | 3 | 4.005 us | 7.027 us | 1.75x | 0.000 s | 2.7 s | ~320x |
| `kidscore_momhs` | 3 | 1.524 us | 2.668 us | 1.75x | 0.000 s | 2.8 s | ~810x |
| `logearn_height` | 3 | 4.041 us | 7.039 us | 1.74x | 0.001 s | 2.8 s | ~330x |
| `bym2_offset_only` | 3845 | 35.551 us | 61.203 us | 1.72x | 0.002 s | 4.2 s | ~59x |
| `kidscore_momiq` | 3 | 1.520 us | 2.596 us | 1.71x | 0.000 s | 2.8 s | ~820x |
| `logmesquite_logvolume` | 3 | 246 ns | 412 ns | 1.67x | 0.000 s | 2.9 s | ~3510x |
| `accel_gp` | 66 | 4.603 us | 7.684 us | 1.67x | 0.004 s | 6.3 s | ~500x |
| `hmm_example` | 4 | 15.694 us | 26.102 us | 1.66x | 0.020 s | 3.9 s | ~77x |
| `kilpisjarvi` | 3 | 308 ns | 504 ns | 1.64x | 0.000 s | 2.7 s | ~2960x |
| `lsat_model` | 1006 | 37.541 us | 59.762 us | 1.59x | 0.002 s | 3.6 s | ~48x |
| `2pl_latent_reg_irt` | 531 | 64.400 us | 100.558 us | 1.56x | 0.004 s | 5.5 s | ~43x |
| `hmm_gaussian` | 14 | 167.781 us | 261.635 us | 1.56x | 0.326 s | 4.6 s | ~7.7x |
| `accel_splines` | 82 | 4.758 us | 7.370 us | 1.55x | 0.003 s | 4.3 s | ~340x |
| `hier_2pl` | 669 | 189.601 us | 290.582 us | 1.53x | 0.005 s | 6.4 s | ~18x |
| `covid19imperial_v3` | 51 | 231.786 us | 340.314 us | 1.47x | 0.653 s | 6.5 s | ~6.4x |
| `wells_dist` | 2 | 21.102 us | 30.838 us | 1.46x | 0.001 s | 2.9 s | ~68x |
| `covid19imperial_v2` | 51 | 232.781 us | 337.668 us | 1.45x | 0.649 s | 6.5 s | ~6.4x |
| `seeds_model` | 26 | 687 ns | 994 ns | 1.45x | 0.000 s | 3.5 s | ~1980x |
| `prophet` | 62 | 35.013 us | 50.617 us | 1.45x | 0.008 s | 5.3 s | ~69x |
| `seeds_stanified_model` | 26 | 688 ns | 993 ns | 1.44x | 0.000 s | 3.5 s | ~2000x |
| `Rate_4_model` | 2 | 104 ns | 145 ns | 1.39x | 0.000 s | 2.4 s | ~5510x |
| `GLM_Binomial_model` | 3 | 799 ns | 1.067 us | 1.34x | 0.000 s | 3.1 s | ~1550x |
| `eight_schools_centered` | 10 | 218 ns | 290 ns | 1.33x | 0.000 s | 2.8 s | ~4120x |
| `Rate_1_model` | 1 | 73 ns | 96 ns | 1.32x | 0.000 s | 2.3 s | ~6790x |
| `eight_schools_noncentered` | 10 | 204 ns | 268 ns | 1.31x | 0.000 s | 3.1 s | ~4570x |
| `Rate_5_model` | 1 | 86 ns | 112 ns | 1.30x | 0.000 s | 2.4 s | ~5970x |
| `Rate_2_model` | 2 | 128 ns | 166 ns | 1.30x | 0.000 s | 2.4 s | ~4460x |
| `irt_2pl` | 144 | 16.514 us | 21.318 us | 1.29x | 0.001 s | 4.0 s | ~120x |
| `hmm_drive_1` | 6 | 111.147 us | 143.476 us | 1.29x | 0.093 s | 4.5 s | ~15x |
| `pilots` | 18 | 597 ns | 767 ns | 1.28x | 0.000 s | 3.3 s | ~2020x |
| `Rate_3_model` | 1 | 90 ns | 114 ns | 1.27x | 0.000 s | 2.3 s | ~5560x |
| `surgical_model` | 14 | 448 ns | 567 ns | 1.27x | 0.000 s | 3.2 s | ~2670x |
| `hmm_drive_0` | 6 | 104.554 us | 132.068 us | 1.26x | 0.092 s | 4.6 s | ~16x |
| `bones_model` | 13 | 40.522 us | 50.060 us | 1.24x | 0.010 s | 3.4 s | ~38x |
| `logistic_regression_rhs` | 3075 | 39.751 us | 48.042 us | 1.21x | 0.012 s | 4.8 s | ~54x |
| `Mb_model` | 3 | 41.839 us | 50.289 us | 1.20x | 0.019 s | 3.1 s | ~31x |
| `Survey_model` | 1 | 54.063 us | 60.867 us | 1.13x | 0.006 s | 2.8 s | ~26x |
| `kronecker_gp` | 438 | 191.959 us | 215.956 us | 1.13x | 0.007 s | 9.0 s | ~24x |
| `hierarchical_gp` | 933 | 36.646 us | 41.217 us | 1.12x | 0.012 s | 8.6 s | ~100x |
| `nn_rbm1bJ10` | 7951 | 170.120 us | 186.226 us | 1.09x | 0.009 s | 5.2 s | ~16x |
| `nn_rbm1bJ100` | 79411 | 414.283 ms | 435.302 ms | 1.05x | 3.001 s | 5.1 s | ~1.1x |
| `wells_dist100ars_model` | 3 | 17.174 us | 17.935 us | 1.04x | 0.002 s | 3.0 s | ~85x |
| `nes_logit_model` | 2 | 6.109 us | 6.372 us | 1.04x | 0.000 s | 3.0 s | ~240x |
| `wells_dae_c_model` | 5 | 17.580 us | 18.294 us | 1.04x | 0.002 s | 3.2 s | ~88x |
| `wells_dist100_model` | 2 | 15.951 us | 16.559 us | 1.04x | 0.001 s | 3.0 s | ~91x |
| `wells_interaction_model` | 4 | 18.782 us | 19.458 us | 1.04x | 0.002 s | 3.1 s | ~80x |
| `wells_daae_c_model` | 6 | 19.204 us | 19.894 us | 1.04x | 0.002 s | 3.2 s | ~81x |
| `wells_dae_model` | 4 | 18.689 us | 19.358 us | 1.04x | 0.002 s | 3.1 s | ~81x |
| `wells_dae_inter_model` | 7 | 19.533 us | 20.176 us | 1.03x | 0.002 s | 3.2 s | ~79x |
| `wells_interaction_c_model` | 4 | 18.771 us | 19.340 us | 1.03x | 0.002 s | 3.1 s | ~80x |
| `one_comp_mm_elim_abs` | 4 | 456.829 us | 461.079 us | 1.01x | 0.001 s | 3.6 s | ~4.9x |
| `diamonds` | 26 | 31.334 us | 31.017 us | 0.99x | 0.022 s | 3.2 s | ~39x |
| `garch11` | 4 | 6.890 us | 6.597 us | 0.96x | 0.002 s | 2.8 s | ~180x |
| `arma11` | 4 | 4.247 us | 3.942 us | 0.93x | 0.002 s | 2.8 s | ~270x |
| `blr` | 6 | 507 ns | 430 ns | 0.85x | 0.000 s | 3.0 s | ~2110x |
| `gp_regr` | 3 | 5.578 us | 3.182 us | 0.57x | 0.000 s | 5.1 s | ~450x |
| `gp_pois_regr` | 13 | 5.266 us | 2.518 us | 0.48x | 0.000 s | 5.4 s | ~500x |

| model | stanli gradient | CmdStan gradient | gradient speedup | what stopped it |
| --- | ---: | ---: | ---: | --- |
| `sir` | - | - | - | stanli's gradient probe threw at the benchmark point; the CmdStan gradient driver would not run; no stanli compile time; no stanli gradient |

## Benchmark method

This section describes the method behind the tables above; the stanc3
optimizer and loop vectorizer comparison uses the same host and driver, with
its own build and stanc flags described in the previous section.

Measured 2026-09-05 on an Apple M3 Ultra (macOS arm64) with Apple clang 21,
single-threaded, with both engines built at `-O3` and
`-ffp-contract=off`. The stanli columns are one refreshed 120-model run. The
CmdStan columns carry over from a 2026-08-06 run on the same host because no
stanli change can affect them. CmdStan's one-time `make build` setup was
already complete, so its per-model build column uses the warm precompiled
header path and does not include that earlier setup cost.

For gradients, both engines evaluate the sampling log density (proportional
terms plus Jacobian) at the same deterministic unconstrained point. stanli
runs `tools/bench_grad.cpp`; CmdStan runs `tools/bench_cmdstan_grad.cpp` over
the stanc-generated model. CmdStan's driver performs the fresh-vars, gradient,
and memory recovery cycle a leapfrog step performs against that model.
stanli's driver performs the `stan::model::gradient` call the sampler makes
against the executor. The reported cells are warmed arithmetic means from one
timed loop per model.

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

The stanc3 optimizer and loop vectorizer table comes from a separate run
against a patched stanc. The patch is a stanc3 checkout at the pinned commit
with `vectorize_loops` enabled at `--O1` in its optimization level table; see
[`compiler/ocaml/stanli_pipeline.ml`](../compiler/ocaml/stanli_pipeline.ml)
for where stanli's own pipeline does the same.

```sh
python3 harnesses/corpus_bench.py deps/cmdstan deps/posteriordb \
  docs/corpus-bench-o1vec.tsv --cmdstan-stanc PATH_TO_PATCHED_STANC \
  --stancflags --O1 --no-sample --timeout 900
python3 tools/corpus_table.py docs/corpus-bench-o1vec.tsv --o1vec
```
