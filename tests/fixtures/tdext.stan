// Transformed-data While loop (short-circuit guard) and append_row.
data {
  int<lower=1> N;
  vector[N] y;
}
transformed data {
  int i = 1;
  int c = 0;
  matrix[N, 2] A;
  row_vector[2] ar;
  while (i <= N && y[i] > 0) {
    c = c + 1;
    i = i + 1;
  }
  ar = rep_row_vector(1, 2);
  for (j in 1 : N) {
    A[j] = ar * j;
  }
  vector[2 * N] yy = append_row(y, y);
  real s = sum(yy) + c + sum(A[ : , 1]);
}
parameters {
  real mu;
}
model {
  target += normal_lpdf(mu | s, 1);
}
