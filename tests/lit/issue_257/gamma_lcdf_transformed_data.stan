// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
transformed data { real c = gamma_lcdf(1.0 | 2.0, 3.0); }
parameters { real y; }
model { y ~ normal(c, 1); }
