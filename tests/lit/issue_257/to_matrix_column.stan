// STANLI-LIT: XFAIL
// STANLI-LIT-EXPECT: COMPILE_FAIL stanli compile: to_matrix: unknown source shape
parameters { matrix[3, 2] w; }
model {
  to_vector(w) ~ std_normal();
  target += sum(to_matrix(w[, 1]));
}
