// m[1, :] = r: an explicit `:` for the one remaining dimension after a
// single-index prefix, on a plain flat array[2, 3] real (no container leaf
// at all -- unlike idxassign.stan's array-of-matrix). Covers
// unsupported_inline_ode_index_assignment, whose UDF body writes exactly
// this shape (there as `y_approx[1, :] = y_initial`).
functions {
  array[, ] real set_row(array[] real r) {
    array[2, 3] real m;
    m[1, : ] = r;
    for (j in 1 : 3)
      m[2, j] = 0;
    return m;
  }
}
parameters {
  vector[3] r;
}
model {
  array[2, 3] real m = set_row(to_array_1d(r));
  target += sum(m[1]);
  r ~ normal(0, 1);
}
