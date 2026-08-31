// The same four terms as reduce_sum.stan, written without reduce_sum. Stan
// Math without STAN_THREADS makes one whole-slice call, so these two models
// must agree bitwise, not merely to a tolerance.
data {
  int<lower=0> N;
  array[N] real y;
  array[N] real x;
}
transformed data {
  real scale = 0;
  for (i in 1:N) scale += x[i] * 2.0 * (1 + N);
}
parameters {
  real mu;
  real<lower=0> sigma;
  real b;
}
model {
  target += normal_lupdf(y | mu, sigma);
  target += normal_lpdf(y | mu, sigma);
  target += normal_lupdf(y | to_vector(x) * b, 1.0);
  mu ~ normal(scale, 1.0);
}
