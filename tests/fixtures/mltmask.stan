// multiply_lower_tri_self_transpose on the compiled graph. stan-math drops
// the argument's upper triangle before forming the product, so a matrix
// parameter with a non-zero one must not agree with a plain A * A'.
data {
  int<lower=1> M;
}
parameters {
  matrix[M, M] A;
}
model {
  matrix[M, M] P = multiply_lower_tri_self_transpose(A);
  target += P[1, 1] - P[2, 3] + P[3, 2];
  to_vector(A) ~ std_normal();
}
