data {
  int<lower=0> N;
  vector[N] y;
}
parameters {
  real mu;
  real<lower=0> sigma;
}
model {
  for (n in 1:N) {
    if (mu > 0) target += normal_lpdf(y[n] | mu, sigma);
    else target += normal_lpdf(y[n] | -mu, sigma);
  }
}
