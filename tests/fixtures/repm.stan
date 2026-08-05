// rep_matrix on parameters (row-vector across rows, scalar fill) plus
// to_vector flattening.
data {
  int<lower=1> R;
}
parameters {
  vector[3] a;
  real s;
}
model {
  matrix[R, 3] L = rep_matrix(a', R);
  matrix[R, 3] S = rep_matrix(s, R, 3);
  target += normal_lpdf(to_vector(L) | 0, 1);
  target += normal_lpdf(to_vector(S) | 0, 2);
}
