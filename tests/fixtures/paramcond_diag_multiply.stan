// Diagonal matrix products inside a parameter-dependent region use the
// register program's column-major matrix view and preserve it in the result.
parameters {
  matrix[4, 4] m;
  vector[4] left;
  vector[4] right;
  real theta;
}
model {
  if (theta > 0) {
    target += sum(diag_pre_multiply(left, m));
    target += sum(diag_post_multiply(m, right));
    if (theta > 1)
      target += left[1] + right[1];
  } else {
    target += theta;
  }
}
