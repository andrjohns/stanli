# Regenerates tests/brms: make_stancode and make_standata output, written
# out unchanged. Needs brms >= 2.23, mgcv, and mice (the nhanes data).
#
#   Rscript tools/gen_brms_models.R [outdir]

suppressMessages({library(brms); library(mgcv)})
args <- commandArgs(trailingOnly = TRUE)
OUT <- if (length(args)) args[1] else "tests/brms"
dir.create(OUT, showWarnings = FALSE, recursive = TRUE)

## ---- declared dimensionality, read out of the .stan data block --------
# brms standata carries R shapes that do not by themselves say whether a
# length-1 value is a Stan scalar or a one-element vector. The declaration
# does, so the writer reads it.
decl_ndim <- function(code) {
  lines <- strsplit(code, "\n", fixed = TRUE)[[1]]
  i0 <- grep("^\\s*data\\s*\\{", lines)[1]
  if (is.na(i0)) return(list())
  depth <- 0; out <- list()
  for (i in seq(i0, length(lines))) {
    l <- lines[i]
    depth <- depth + lengths(regmatches(l, gregexpr("{", l, fixed = TRUE))) -
                     lengths(regmatches(l, gregexpr("}", l, fixed = TRUE)))
    if (i > i0 && depth <= 0) break
    l <- sub("//.*$", "", l)
    if (!grepl(";", l)) next
    d <- trimws(sub(";.*$", "", l))
    if (d == "" || grepl("^(data|transformed|parameters|model)", d)) next
    # name is the last identifier before ; (ignoring any = default)
    d2 <- sub("=.*$", "", d)
    nm <- regmatches(d2, regexpr("[A-Za-z_][A-Za-z0-9_]*\\s*$", d2))
    if (length(nm) == 0) next
    nm <- trimws(nm)
    ty <- substr(d2, 1, nchar(d2) - nchar(nm))
    n <- 0
    arr <- regmatches(ty, regexpr("^\\s*array\\s*\\[[^]]*\\]", ty))
    if (length(arr) == 1) {
      n <- n + 1 + lengths(regmatches(arr, gregexpr(",", arr, fixed = TRUE)))
      ty <- sub("^\\s*array\\s*\\[[^]]*\\]", "", ty)
    }
    if (grepl("matrix|cov_matrix|corr_matrix|cholesky_factor", ty)) n <- n + 2
    else if (grepl("vector|simplex|ordered|unit_vector", ty)) n <- n + 1
    out[[nm]] <- n
  }
  out
}

num <- function(x) {
  if (is.logical(x)) x <- as.integer(x)
  if (length(x) == 0) return("null")
  if (is.na(x) && !is.nan(x)) return("NaN")          # NA -> NaN token
  if (is.nan(x)) return("NaN")
  if (is.infinite(x)) return(if (x > 0) "Infinity" else "-Infinity")
  if (is.integer(x)) return(format(x, scientific = FALSE, trim = TRUE))
  if (x == round(x) && abs(x) < 1e15)
    return(format(x, digits = 17, scientific = FALSE, trim = TRUE))
  format(x, digits = 17, scientific = TRUE, trim = TRUE)
}

emit <- function(x, ndim) {
  d <- dim(x)
  if (!is.null(d) && length(d) >= 2) {
    # row-major nesting over the leading index
    rows <- lapply(seq_len(d[1]), function(i) {
      sub <- if (length(d) == 2) x[i, ] else
        array(x[i, , , drop = FALSE], dim = d[-1])
      emit(sub, length(d) - 1)
    })
    return(paste0("[", paste(unlist(rows), collapse = ","), "]"))
  }
  x <- as.vector(x)
  if (ndim <= 0 && length(x) == 1) return(num(x))
  paste0("[", paste(vapply(x, num, character(1)), collapse = ","), "]")
}

write_json <- function(sd, code, path) {
  nd <- decl_ndim(code)
  keep <- names(sd)[!vapply(sd, function(v) is.character(v) || is.factor(v) ||
                                            is.list(v), logical(1))]
  parts <- vapply(keep, function(k) {
    v <- sd[[k]]
    dd <- dim(v)
    attributes(v) <- NULL
    if (!is.null(dd) && length(dd) >= 2) dim(v) <- dd
    n <- if (!is.null(nd[[k]])) nd[[k]] else
         if (!is.null(dd)) length(dd) else if (length(v) > 1) 1 else 0
    paste0("\"", k, "\":", emit(v, n))
  }, character(1))
  writeLines(paste0("{", paste(parts, collapse = ","), "}"), path)
}

case <- function(slug, ...) {
  args <- list(...)
  r <- tryCatch({
    code <- do.call(make_stancode, args)
    sd   <- do.call(make_standata, args)
    writeLines(as.character(code), file.path(OUT, paste0(slug, ".stan")))
    write_json(sd, as.character(code), file.path(OUT, paste0(slug, ".json")))
    "ok"
  }, error = function(e) paste("R_FAIL:", conditionMessage(e)))
  cat(sprintf("%-26s %s\n", slug, r))
}

## ---------------- data ----------------
set.seed(1005)
dat_gp <- mgcv::gamSim(eg = 1, n = 30, scale = 2, verbose = FALSE)
data("nhanes", package = "mice")
set.seed(7)
N <- 40
sim <- data.frame(
  x = rnorm(N), z = rnorm(N), g = factor(rep(1:5, each = 8)),
  tt = rep(1:8, 5),
  y = rnorm(N), ypos = rexp(N) + 0.1, cnt = rpois(N, 3),
  bin = rbinom(N, 1, 0.5), tr = rep(10L, N),
  ang = runif(N, -pi, pi), prop = runif(N, 0.05, 0.95),
  cens = sample(c(0L, 1L), N, TRUE),
  wts = runif(N, 0.5, 1.5), se1 = runif(N, 0.1, 0.5),
  mono = factor(sample(1:4, N, TRUE), ordered = TRUE),
  xme = rnorm(N), xsd = runif(N, 0.1, 0.3),
  cat3 = factor(sample(1:3, N, TRUE))
)
sim$succ <- rbinom(N, sim$tr, 0.4)
sim$ymi <- sim$y; sim$ymi[c(3, 9, 17)] <- NA

## ================= issue #319 =================
case("i319_pois_re",      count ~ zAge + zBase * Trt + (1 | patient),
     data = epilepsy, family = poisson())
case("i319_pois_re2",     count ~ zAge + zBase * Trt + (1 | patient) + (1 | obs),
     data = epilepsy, family = poisson())
case("i319_negbin_re",    count ~ zAge + zBase * Trt + (1 | patient),
     data = epilepsy, family = negbinomial())
case("i319_pois_fixed",   count ~ zAge + zBase * Trt,
     data = epilepsy, family = poisson())
case("i319_negbin_fixed", count ~ zAge + zBase * Trt,
     data = epilepsy, family = negbinomial())
case("i319_gauss_re",     count ~ zAge + zBase * Trt + (1 | patient),
     data = epilepsy, family = gaussian())

## ================= issue #320 =================
case("i320_gp_matern32", y ~ gp(x1, x2, cov = "matern32"), data = dat_gp)
case("i320_gp_expquad",  y ~ gp(x1, x2, cov = "exp_quad"), data = dat_gp)
case("i320_sratio_cs",   rating ~ period + carry + cs(treat), data = inhaler,
     family = sratio("logit"), prior = prior(normal(0, 5), class = b))
case("i320_sratio_plain", rating ~ period + carry + treat, data = inhaler,
     family = sratio("logit"), prior = prior(normal(0, 5), class = b))
case("i320_pois_trunc_ub", count | trunc(ub = 104) ~ zBase * Trt,
     data = epilepsy, family = poisson())
case("i320_pois_trunc_both", count | trunc(lb = 0, ub = 104) ~ zBase * Trt,
     data = epilepsy, family = poisson())
case("i320_mi_nhanes",
     bf(bmi | mi() ~ age * mi(chl)) + bf(chl | mi() ~ age) + set_rescor(FALSE),
     data = nhanes)

## ================= wider sweep =================
case("sw_gaussian",    y ~ x + z, data = sim, family = gaussian())
case("sw_student",     y ~ x + z, data = sim, family = student())
case("sw_skewnormal",  y ~ x, data = sim, family = skew_normal())
case("sw_exgaussian",  ypos ~ x, data = sim, family = exgaussian())
case("sw_asymlaplace", y ~ x, data = sim, family = asym_laplace())
case("sw_lognormal",   ypos ~ x, data = sim, family = lognormal())
case("sw_gamma",       ypos ~ x, data = sim, family = Gamma("log"))
case("sw_weibull",     ypos ~ x, data = sim, family = weibull())
case("sw_beta",        prop ~ x, data = sim, family = Beta())
case("sw_vonmises",    ang ~ x, data = sim, family = von_mises())
case("sw_bernoulli",   bin ~ x + z, data = sim, family = bernoulli())
case("sw_binomial",    succ | trials(tr) ~ x, data = sim, family = binomial())
case("sw_poisson",     cnt ~ x + z, data = sim, family = poisson())
case("sw_negbinomial", cnt ~ x + z, data = sim, family = negbinomial())
case("sw_zi_poisson",  cnt ~ x, data = sim, family = zero_inflated_poisson())
case("sw_zi_negbin",   cnt ~ x, data = sim, family = zero_inflated_negbinomial())
case("sw_zi_binomial", succ | trials(tr) ~ x, data = sim,
     family = zero_inflated_binomial())
case("sw_hurdle_pois", cnt ~ x, data = sim, family = hurdle_poisson())
case("sw_hurdle_gamma", ypos ~ x, data = sim, family = hurdle_gamma())
case("sw_hurdle_lognormal", ypos ~ x, data = sim, family = hurdle_lognormal())
case("sw_cumulative",  rating ~ period + carry, data = inhaler,
     family = cumulative())
case("sw_sratio",      rating ~ period, data = inhaler, family = sratio())
case("sw_cratio",      rating ~ period, data = inhaler, family = cratio())
case("sw_acat",        rating ~ period, data = inhaler, family = acat())
case("sw_cumulative_cs", rating ~ period + cs(carry), data = inhaler,
     family = cumulative())
case("sw_cratio_cs",   rating ~ period + cs(carry), data = inhaler,
     family = cratio())
case("sw_acat_cs",     rating ~ period + cs(carry), data = inhaler,
     family = acat())
case("sw_categorical", cat3 ~ x, data = sim, family = categorical())
case("sw_re_slope",    y ~ x + (1 + x | g), data = sim)
case("sw_mono",        y ~ mo(mono), data = sim)
case("sw_me",          y ~ me(xme, xsd), data = sim)
case("sw_mi",          bf(ymi | mi() ~ x) + bf(x ~ z) + set_rescor(FALSE),
     data = sim)
case("sw_spline_s",    y ~ s(x), data = sim)
case("sw_spline_t2",   y ~ t2(x, z), data = sim)
case("sw_gp",          y ~ gp(x), data = sim)
case("sw_ar",          y ~ x + ar(tt, g), data = sim)
case("sw_ma",          y ~ x + ma(tt, g), data = sim)
case("sw_arma",        y ~ x + arma(tt, g), data = sim)
case("sw_mv_rescor",   bf(mvbind(y, ypos) ~ x) + set_rescor(TRUE), data = sim)
case("sw_mv_norescor", bf(mvbind(y, ypos) ~ x) + set_rescor(FALSE), data = sim)
case("sw_nonlinear",
     bf(ypos ~ a * exp(b * x), a + b ~ 1, nl = TRUE), data = sim,
     prior = c(prior(normal(1, 2), nlpar = "a"),
               prior(normal(0, 1), nlpar = "b")))
case("sw_mixture",     y ~ x, data = sim, family = mixture(gaussian, gaussian))
case("sw_dist_sigma",  bf(y ~ x, sigma ~ z), data = sim)
case("sw_trunc",       ypos | trunc(lb = 0, ub = 20) ~ x, data = sim)
case("sw_cens",        ypos | cens(cens) ~ x, data = sim)
case("sw_weights",     y | weights(wts) ~ x, data = sim)
case("sw_se",          y | se(se1) ~ x, data = sim)
case("sw_re_pois",     cnt ~ x + (1 | g), data = sim, family = poisson())
case("sw_re_bern",     bin ~ x + (1 | g), data = sim, family = bernoulli())
case("sw_re_negbin",   cnt ~ x + (1 | g), data = sim, family = negbinomial())
case("sw_re_gauss",    y ~ x + (1 | g), data = sim, family = gaussian())

