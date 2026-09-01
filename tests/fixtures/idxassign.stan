// H[1, :, :] = seed * theta: an explicit `:` for every leaf dimension after
// a full array-index prefix, spelling the same whole-element replacement as
// H[1] = seed * theta. Covers unsupported_indexed_assignment, which this
// mirrors (same statements, same H layout) with a proper posterior.
parameters {
  real theta;
}
transformed parameters {
  matrix[2, 2] seed = diag_matrix(rep_vector(1.0, 2));
  array[2] matrix[2, 2] H;
  H[1] = seed;
  H[2] = seed;
  H[1, : , :] = seed * theta;
}
model {
  for (i in 1 : 2)
    to_vector(H[i]) ~ normal(0, 1);
  theta ~ normal(0, 1);
}
