// STANLI-LIT: XFAIL
// STANLI-LIT-EXPECT: COMPILE_FAIL stanli compile: unsupported function columns_dot_self
parameters { vector[4] y; }
model {
  y ~ std_normal();
  target += sum(columns_dot_self(to_matrix(y, 2, 2)));
}
