// STANLI-LIT: XFAIL
// STANLI-LIT-EXPECT: COMPILE_FAIL stanli prepare_data: unsupported function gamma_lcdf
transformed data { real c = gamma_lcdf(1.0 | 2.0, 3.0); }
parameters { real y; }
model { y ~ normal(c, 1); }
