// Phase 0 characterization: a matrix row is a strided Eigen view. The
// current generated-quantities native extrema surface deliberately refuses
// it, so the write_array path must stay on WaInterp.
data {
  int<lower=1> N;
}
parameters {
  matrix[2, N] x;
}
model {
  to_vector(x) ~ normal(0, 1);
}
generated quantities {
  real row_min = min(x[1]);
}
