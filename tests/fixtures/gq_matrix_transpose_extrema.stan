parameters {
  matrix[5, 2] m;
}
model {
  to_vector(m) ~ normal(0, 1);
}
generated quantities {
  // Eigen's matrix-transpose evaluator is not equivalent to reducing a
  // materialized col-major transpose slot with the input matrix's layout.
  real transpose_min = min(m');
}
