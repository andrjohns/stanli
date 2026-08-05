// append_row stacking parameter matrices (col-major interleave) and a
// row_vector onto a matrix.
data { int<lower=1> C; }
parameters {
  matrix[2, C] A;
  matrix[1, C] B;
  row_vector[C] r;
}
model {
  matrix[3, C] S = append_row(A, B);
  matrix[3, C] T = append_row(r, A);
  target += normal_lpdf(to_vector(S) | 0, 1);
  target += normal_lpdf(to_vector(T) | 0, 2);
}
