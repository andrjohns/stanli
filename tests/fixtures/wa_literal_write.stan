parameters {
  real mu;
}
model {
  mu ~ normal(0, 1);
}
generated quantities {
  real two = 2;
}
