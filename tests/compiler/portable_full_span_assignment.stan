data {
  int<lower=0> N;
  vector[N] x;
}
transformed data {
  vector[N] y;
  y[:] = x;
}
parameters {
  real z;
}
model {
  z ~ normal(sum(y), 1);
}
