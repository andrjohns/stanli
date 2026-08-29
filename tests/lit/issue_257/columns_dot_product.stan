// STANLI-LIT: XFAIL
// STANLI-LIT-EXPECT: COMPILE_FAIL stanli compile: unsupported function columns_dot_product
// STANLI-LIT-DATA: {"A": [[1, 2], [3, 4]]}
data { matrix[2, 2] A; }
parameters { vector[4] y; }
model {
  y ~ std_normal();
  target += sum(columns_dot_product(to_matrix(y, 2, 2), A));
}
