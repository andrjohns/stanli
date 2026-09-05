# Models from brms

brms writes a large share of the Stan code that people actually run, and
its output is a corpus posteriordb does not contain: `lprior`
accumulation, the `scale_r_cor` helper, `r_1_1[J_1[n]] * Z_1_1[n]`
indexing, spline and monotonic blocks, the inlined ordinal lpmfs, the
`mi()` imputation shapes. posteriordb carries one brms model, `diamonds`,
from brms 2.10.0.

Every `.stan` and `.json` here is `make_stancode` and `make_standata`
output from brms 2.23.0, written out unchanged.
[`tools/gen_brms_models.R`](../../tools/gen_brms_models.R) is the
generator; it needs brms, mgcv and mice, and rerunning it reproduces this
directory byte for byte.

These models go through the same oracle as the corpus: CmdStan's recorded
log density and full gradient in `docs/corpus-refs.json.gz`, replayed by
[`tools/verify_refs.py`](../../tools/verify_refs.py) in CI on every push.
`tools/verify_refs.py` finds a model here by name before it looks in
posteriordb, so nothing about the CI step changed.

## What they cover

The `i319_` and `i320_` models are the ones reported in issues #319 and
#320, with the neighbouring variants that isolate what each report turns
on: a Poisson fit with and without a group effect, a truncation with and
without a lower bound, an ordinal fit with and without `cs()`.

The `sw_` models are a sweep over what brms generates elsewhere: the
response families (gaussian, student, skew normal, ex-Gaussian,
asymmetric Laplace, lognormal, gamma, Weibull, beta, von Mises,
Bernoulli, binomial, Poisson, negative binomial, their zero-inflated and
hurdle forms, the four ordinal families, categorical and mixture), the
effect structures (varying intercepts, varying slopes, monotonic effects,
measurement error, missing-data imputation, `s()` and `t2()` splines,
Gaussian processes, autocorrelation terms, multivariate responses with
and without residual correlation, distributional and nonlinear
formulas), and the addition terms (`trunc`, `cens`, `weights`, `se`,
`trials`).

brms emits the same code for `y ~ x + (1 | g)`, for the same formula
under `family = gaussian()`, and for `gr(g, cor = FALSE)` on a single
term, so `sw_re_gauss` stands for all three.

## The data

`make_standata` output is written as JSON by the generator, and its
values are kept as they come. Three models carry non-finite ones:
`i320_pois_trunc_ub` truncates from above only and brms fills the lower
bound with `-Infinity`, and `i320_mi_nhanes` and `sw_mi` carry
`Infinity` for the responses they impute and for the imputation bounds.
CmdStan's JSON parser reads those tokens; stanli's refuses them, which is
one of the gaps below.

## Known gaps

Thirteen models do not run. Their references are recorded from CmdStan
the same way every other model's are, and
`KNOWN_GAPS` in [`tools/verify_refs.py`](../../tools/verify_refs.py)
holds the list of models whose failure is expected. When a gap closes the
model matches its references, the replay reports `GAP_CLOSED`, and the
run stays red until the entry is deleted.

| what stops it | models |
| --- | --- |
| a vector `alpha` in `poisson_log_glm`, `neg_binomial_2_log_glm` and `bernoulli_logit_glm`, which is what a group effect becomes | `i319_pois_re`, `i319_pois_re2`, `i319_negbin_re`, `sw_re_pois`, `sw_re_negbin`, `sw_re_bern` |
| `gp_matern32_cov` | `i320_gp_matern32` |
| `num_elements` of an expression inside a runtime-control region, which is how the inlined `sratio` and `cratio` lpmfs read their threshold count | `i320_sratio_cs`, `sw_cratio_cs` |
| non-finite values in the data file | `i320_pois_trunc_ub`, `i320_mi_nhanes`, `sw_mi` |
| an array-valued location in `multi_normal_cholesky` | `sw_mv_rescor` |

## Regenerating and recording

```
Rscript tools/gen_brms_models.R
python3 tools/verify_sample.py deps/cmdstan deps/posteriordb sw_gaussian
```

The recorder prints one line per evaluation point and writes a reference
at each of them; commit `docs/corpus-refs.json.gz` and
`docs/verification.json` with the model. A point CmdStan refuses is
recorded as a refusal, and one where stanli disagrees is recorded anyway,
because references describe CmdStan. Read the per-point lines before
committing: a `MISMATCH` on a new model is a finding.

## What these found

`sw_gp` and `i320_gp_expquad` are recorded as `MISMATCH` at two of their
three points. Their log density and every gradient are bitwise identical
to CmdStan except for the two GP hyperparameters, `sdgp` and `lscale`,
which differ by 1.2e-8 and 2.8e-9 relative. Both flow through
`cholesky_decompose` of the exponentiated-quadratic covariance, which
brms holds up with a 1e-12 jitter on the diagonal. At the shared
evaluation point the smallest Cholesky pivot of that covariance is
1.1e-12 against a diagonal of 1, so the factorization is singular to
machine precision and its derivative divides by that pivot. This is the
same amplification `kronecker_gp` shows in the posteriordb corpus.
