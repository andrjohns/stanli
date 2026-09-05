// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
// STANLI-LIT-DATA: {"x": Infinity}
data { real<lower=0> x; }
parameters { real mu; }
model { mu ~ normal(0, 1); }
