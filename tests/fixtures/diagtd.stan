data {
  vector[3] diagonal;
}
transformed data {
  matrix[3, 3] diagonal_matrix = diag_matrix(diagonal);
}
parameters {
  real theta;
}
model {
  theta ~ normal(diagonal_matrix[2, 2] + diagonal_matrix[1, 2], 1);
}
