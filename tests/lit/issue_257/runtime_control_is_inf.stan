// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
parameters { real y; }
model {
  y ~ std_normal();
  if (!is_inf(y)) target += y;
}
