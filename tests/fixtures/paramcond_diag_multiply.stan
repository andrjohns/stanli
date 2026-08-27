// Diagonal matrix products inside a parameter-dependent region use the
// register program's column-major matrix view and preserve it in the result.
parameters {
  matrix[2, 3] m;
  vector[2] left;
  vector[3] right;
  real theta;
}
model {
  if (theta > 0) {
    target += sum(diag_pre_multiply(left, m));
    target += sum(diag_post_multiply(m, right));
  } else {
    target += theta;
  }
}
