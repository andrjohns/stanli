// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
parameters { matrix[3, 2] w; }
model {
  to_vector(w) ~ std_normal();
  target += sum(to_matrix(w[, 1]));
}
