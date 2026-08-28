data {
  vector[3] diagonal;
  matrix[3, 2] rectangular;
}
transformed data {
  matrix[3, 3] diagonal_matrix = diag_matrix(diagonal);
  vector[2] extracted = diagonal(rectangular);
}
parameters {
  real theta;
}
model {
  theta ~ normal(diagonal_matrix[2, 2] + diagonal_matrix[1, 2] + extracted[2], 1);
}
