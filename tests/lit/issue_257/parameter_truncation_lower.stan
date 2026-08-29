// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
parameters { real sigma; }
model { sigma ~ cauchy(0, 1) T[0, ]; }
