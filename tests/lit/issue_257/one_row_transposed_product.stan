// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
// STANLI-LIT-DATA: {"D": 3, "N": 1}
data { int D; int N; }
parameters { matrix[D, N] z; matrix[D, D] L; }
transformed parameters {
  matrix[N, D] theta;
  theta = (L * z)';
}
model { to_vector(z) ~ std_normal(); to_vector(L) ~ std_normal(); }
