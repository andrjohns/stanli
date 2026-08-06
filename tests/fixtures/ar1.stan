// Stationary AR(1) prior, sampled directly. Every marginal is standard
// normal and corr(x[i], x[j]) = rho^|i-j|, so the draws have known moments --
// but the target is badly conditioned in a way a diagonal metric cannot
// remove, so NUTS needs trajectories far longer than 31 leapfrog steps to
// traverse it. That makes it the regression guard for max tree depth.
data {
  int<lower=2> K;
  real<lower=0, upper=1> rho;
}
transformed data {
  real sigma_e = sqrt(1 - rho * rho);
}
parameters {
  vector[K] x;
}
model {
  x[1] ~ normal(0, 1);
  x[2:K] ~ normal(rho * x[1:K - 1], sigma_e);
}
