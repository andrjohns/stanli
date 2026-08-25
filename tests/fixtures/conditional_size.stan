data {
  int<lower=0, upper=1> enabled;
  int<lower=0> n;
  matrix[enabled ? n : 0, enabled ? 2 : 0] x;
}
parameters {
  real theta;
  vector[(enabled && n > 0) ? n : 0] beta;
  vector[(enabled || n == 0) ? 1 : 0] marker;
}
model {
  theta ~ normal(0, 1);
  beta ~ normal(0, 1);
  marker ~ normal(0, 1);
}
