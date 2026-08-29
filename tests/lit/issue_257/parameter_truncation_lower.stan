// STANLI-LIT: XFAIL
// STANLI-LIT-EXPECT: COMPILE_FAIL stanli compile: runtime-control region: internal function FnNegInf
parameters { real sigma; }
model { sigma ~ cauchy(0, 1) T[0, ]; }
