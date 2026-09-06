parameters {
  real mu;
}
model {
  mu ~ normal(0, 1);
}
generated quantities {
  real s = 0;
  if (mu > 0) {
    matrix[1100, 1000] big = rep_matrix(mu, 1100, 1000);
    s = big[1, 1];
  }
}
