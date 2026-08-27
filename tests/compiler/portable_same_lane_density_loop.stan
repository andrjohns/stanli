data {
  int<lower=0> N;
  array[N] int<lower=1, upper=N> group;
  vector[N] alpha;
  vector[N] x;
  vector[N] y;
}
parameters {
  real beta;
  real<lower=0> sigma;
}
model {
  vector[N] mu;
  for (n in 1:N) {
    mu[n] = alpha[group[n]] + beta * x[n];
    target += normal_lpdf(y[n] | mu[n], sigma);
  }
}
