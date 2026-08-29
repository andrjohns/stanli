// STANLI-LIT: XFAIL
// STANLI-LIT-EXPECT: COMPILE_FAIL stanli compile: runtime-control region: internal function FnNegInf
parameters { real sigma; }
model { sigma ~ normal(0, 1) T[-1, 1]; }
