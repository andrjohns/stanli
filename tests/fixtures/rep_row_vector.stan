parameters {
  real theta;
}
model {
  row_vector[4] x = rep_row_vector(theta, 4);
  x ~ normal(0, 1);
}
