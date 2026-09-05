// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: COMPILE_FAIL block: nrows is -1, but must be nonnegative!
transformed data {
  matrix[3, 3] m = rep_matrix(1.0, 3, 3);
  real s = sum(block(m, 3, 1, -1, 1));
}
parameters {
  real theta;
}
model {
  theta ~ std_normal();
}
