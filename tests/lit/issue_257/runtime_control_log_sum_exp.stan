// STANLI-LIT: XFAIL
// STANLI-LIT-EXPECT: COMPILE_FAIL stanli compile: runtime-control region: function log_sum_exp
parameters { vector[2] y; }
model {
  y ~ std_normal();
  if (y[1] > 0) target += log_sum_exp(y);
}
