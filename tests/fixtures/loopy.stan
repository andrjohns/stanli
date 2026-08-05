data {
  int<lower=0> N;
  vector[N] y;
}
parameters {
  real mu;
}
model {
  for (n in 1:N) {
    y[n] ~ normal(mu, 1);
  }
}
