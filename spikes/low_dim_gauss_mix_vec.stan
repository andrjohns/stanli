data {
  int<lower=0> N;
  vector[N] y;
}
parameters {
  ordered[2] mu;
  array[2] real<lower=0> sigma;
  real<lower=0, upper=1> theta;
}
model {
  sigma ~ normal(0, 2);
  mu ~ normal(0, 2);
  theta ~ beta(5, 5);

  // Partially re-rolled: the two normal_lpdf chains vectorize; the per-n
  // log_mix stays scalar (log_mix(t, l1, l2) = log_sum_exp(log(t) + l1,
  // log1m(t) + l2)). The shared -0.5*log(2*pi()) per-element constant is
  // added once at the end.
  vector[N] la = log(theta) - log(sigma[1])
                 - 0.5 * square((y - mu[1]) / sigma[1]);
  vector[N] lb = log1m(theta) - log(sigma[2])
                 - 0.5 * square((y - mu[2]) / sigma[2]);
  for (n in 1 : N) {
    target += log_sum_exp(la[n], lb[n]);
  }
  // log(2*pi) as a literal: stanrt does not lower pi()
  target += -0.5 * N * 1.8378770664093454836;
}
