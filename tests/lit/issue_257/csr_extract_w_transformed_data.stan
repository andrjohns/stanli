// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
transformed data {
  vector[4] w = csr_extract_w([[1.0, 2.0], [3.0, 4.0]]);
}
parameters { real y; }
model { y ~ normal(w[1], 1); }
