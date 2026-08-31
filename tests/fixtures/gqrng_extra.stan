// gumbel_rng / dirichlet_rng / beta_binomial_rng via the WaInterp fallback.
data {
  int<lower=1> N;
}
parameters {
  real<lower=0> sigma;
}
model {
  sigma ~ normal(0, 1);
}
generated quantities {
  real g = gumbel_rng(0.5, sigma);
  vector[3] d = dirichlet_rng([1.0, 2.0, 3.0]');
  int bb = beta_binomial_rng(N, 2.0, 3.0);
}
