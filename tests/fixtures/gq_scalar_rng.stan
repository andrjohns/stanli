parameters {
  real x;
}
model {
  x ~ normal(0, 1);
}
generated quantities {
  int p = poisson_log_rng(0.3);
  real u = uniform_rng(-2, 3);
  int b = bernoulli_rng(0.4);
  real n = normal_rng(x, 1.2);
  real l = lognormal_rng(0.2, 0.7);
  int<lower=2, upper=7> k = 2 + binomial_rng(5, inv_logit(x));
  real gm = gumbel_rng(0.5, 1.3);
  real ex = exponential_rng(1.6);
}
