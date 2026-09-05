// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: greater than or equal
// STANLI-LIT-DATA: {"x": NaN}
data { real<lower=0> x; }
parameters { real mu; }
model { mu ~ normal(0, 1); }
