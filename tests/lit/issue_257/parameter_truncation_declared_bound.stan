// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
parameters { real<lower=0> sigma; }
model { sigma ~ cauchy(0, 1) T[0, ]; }
