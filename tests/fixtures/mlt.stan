// multiply_lower_tri_self_transpose in a runtime-control region -> WaInterp.
data {
  int<lower=1> M;
}
parameters {
  matrix[M, M] A;
}
generated quantities {
  matrix[M, M] P = rep_matrix(0, M, M);
  int i = 1;
  while (i <= 1) {
    P = multiply_lower_tri_self_transpose(A);
    i += 1;
  }
}
