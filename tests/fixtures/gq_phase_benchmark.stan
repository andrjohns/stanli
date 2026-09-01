data {
  int<lower=2> N;
}
parameters {
  vector[N] x;
}
model {
  x ~ normal(0, 1);
}
generated quantities {
  real tail_product = prod(x[2 : N]);
  real tail_minimum = min(x[2 : N]);
  real tail_maximum = max(x[2 : N]);
}
