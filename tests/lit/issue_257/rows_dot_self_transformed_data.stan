// STANLI-LIT: XFAIL
// STANLI-LIT-EXPECT: COMPILE_FAIL stanli prepare_data: unsupported function rows_dot_self
transformed data {
  matrix[2, 2] A = [[1, 2], [3, 4]];
  vector[2] r = rows_dot_self(A);
}
parameters { real y; }
model { y ~ normal(r[1], 1); }
