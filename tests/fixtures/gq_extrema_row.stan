// A matrix row is a strided Eigen view. The native path retains its scalar
// coefficient traversal for the generated-quantities reduction.
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
