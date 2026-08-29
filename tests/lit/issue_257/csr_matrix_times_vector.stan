// STANLI-LIT: XFAIL
// STANLI-LIT-EXPECT: COMPILE_FAIL stanli compile: unsupported function csr_matrix_times_vector
// STANLI-LIT-DATA: {"N": 2, "w": [1, 2, 3], "v": [1, 2, 1], "u": [1, 3, 4]}
data {
  int N;
  vector[3] w;
  array[3] int v;
  array[3] int u;
}
parameters { vector[N] y; }
model {
  y ~ std_normal();
  target += sum(csr_matrix_times_vector(2, N, w, v, u, y));
}
