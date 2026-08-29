// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
parameters { vector[2] y; }
model {
  y ~ std_normal();
  if (y[1] > 0) target += log_sum_exp(y);
}
