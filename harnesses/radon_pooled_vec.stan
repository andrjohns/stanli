data {
  int<lower=0> N;
  vector[N] floor_measure;
  vector[N] log_radon;
}
parameters {
  real alpha;
  real beta;
  real<lower=0> sigma_y;
}
model {
  vector[N] mu;

  // priors
  sigma_y ~ normal(0, 1);
  alpha ~ normal(0, 10);
  beta ~ normal(0, 10);

  // likelihood: the re-rolled form of the original's per-n loop
  mu = alpha + beta * floor_measure;
  target += normal_lpdf(log_radon | mu, sigma_y);
}
