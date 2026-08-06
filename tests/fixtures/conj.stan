// Conjugate normal with flat priors: the one model whose posterior we know
// in closed form, so sampled draws can be checked against arithmetic rather
// than against another sampler.
//
// Neither parameter is given a prior, so both are uniform on the constrained
// scale -- sigma flat on (0, inf), which is what the `<lower=0>` jacobian
// buys. With ss = sum((y - ybar)^2) the posterior is then
//   1/sigma^2 | y ~ Gamma((N-2)/2, rate = ss/2)
//   mu_c      | y ~ t_{N-2} with variance ss / (N (N-4))
// Both have elementary moments, which is what tests/test_sampling.cpp
// asserts.
//
// It also carries one of everything the write_array path has to handle: a
// transformed data block the likelihood reads, a transformed parameter, and
// generated quantities that are exact functions of the draw (so they can be
// checked per draw, not just in distribution).
data {
  int<lower=4> N;
  vector[N] y;
}
transformed data {
  real ybar = mean(y);
  vector[N] yc = y - ybar;
}
parameters {
  real mu_c;
  real<lower=0> sigma;
}
transformed parameters {
  real prec = 1 / (sigma * sigma);
}
model {
  yc ~ normal(mu_c, sigma);
}
generated quantities {
  real mu = mu_c + ybar;
  real sd_from_prec = sqrt(1 / prec);
  vector[N] resid = yc - mu_c;
}
