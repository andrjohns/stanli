// Transformed-data While loop (short-circuit guard) and append_row.
data {
  int<lower=1> N;
  vector[N] y;
}
transformed data {
  int i = 1;
  int c = 0;
  while (i <= N && y[i] > 0) {
    c = c + 1;
    i = i + 1;
  }
  vector[2 * N] yy = append_row(y, y);
  real s = sum(yy) + c;
}
parameters {
  real mu;
}
model {
  target += normal_lpdf(mu | s, 1);
}
