// The same call inside a while loop, which makes the whole body a
// runtime-control region on the register machine: its var replay has to
// reproduce both the upper-triangle mask and stan-math's triangular
// pullback, not the scalar sums a Gram product would expand into.
data {
  int<lower=1> M;
}
parameters {
  matrix[M, M] A;
}
model {
  real acc = 0;
  int i = 1;
  while (i <= 2) {
    matrix[M, M] P = multiply_lower_tri_self_transpose(A);
    acc += P[1, 1] - P[2, 3] + P[3, 2];
    i += 1;
  }
  target += acc;
  to_vector(A) ~ std_normal();
}
