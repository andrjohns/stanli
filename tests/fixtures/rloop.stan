data {
  int<lower=0> N;
  vector[N] x;
  vector[N] y;
}
parameters {
  real alpha;
  real beta;
  real<lower=0> sigma;
}
model {
  vector[N] mu;
  sigma ~ normal(0, 1);
  alpha ~ normal(0, 10);
  beta ~ normal(0, 10);
  mu = alpha + beta * x;
  for (n in 1 : N) {
    target += normal_lpdf(y[n] | mu[n], sigma);
  }
}
