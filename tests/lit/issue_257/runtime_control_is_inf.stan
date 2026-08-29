// STANLI-LIT: XFAIL
// STANLI-LIT-EXPECT: COMPILE_FAIL stanli compile: runtime-control region: function is_inf
parameters { real y; }
model {
  y ~ std_normal();
  if (!is_inf(y)) target += y;
}
