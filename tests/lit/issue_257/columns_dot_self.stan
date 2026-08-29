// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
parameters { vector[4] y; }
model {
  y ~ std_normal();
  target += sum(columns_dot_self(to_matrix(y, 2, 2)));
}
