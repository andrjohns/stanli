// choose() in the three integer positions brms writes it in: a transformed
// data int that sizes a parameter, a generated quantities declaration
// extent, and an index whose argument is an unrolled loop variable.
data {
  int M;
}
transformed data {
  int nc = choose(M, 2);
}
parameters {
  vector[nc] z;
  matrix[M, M] A;
}
model {
  z ~ std_normal();
  to_vector(A) ~ std_normal();
}
generated quantities {
  vector[choose(M, 2)] cor;
  for (k in 1 : M) {
    for (j in 1 : (k - 1)) {
      cor[choose(k - 1, 2) + j] = A[j, k];
    }
  }
}
