// STANLI-LIT: XFAIL
// STANLI-LIT-EXPECT: COMPILE_FAIL stanli prepare_data: unsupported function rows_dot_product
transformed data {
  matrix[2, 2] A = [[1, 2], [3, 4]];
  matrix[2, 2] B = [[5, 6], [7, 8]];
  vector[2] r = rows_dot_product(A, B);
}
parameters { real y; }
model { y ~ normal(r[1], 1); }
