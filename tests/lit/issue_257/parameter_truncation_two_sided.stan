// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
parameters { real sigma; }
model { sigma ~ normal(0, 1) T[-1, 1]; }
