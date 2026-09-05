functions {
  real gathered_trace(matrix m) {
    int d = rows(m);
    matrix[d, d] o = rep_matrix(0, d, d);
    for (i in 1:d) o[i, i] = 1;
    return sum(o) + sum(m);
  }
}
data {
  int<lower=1> K;
  int<lower=1, upper=K> n;
  array[n] int<lower=1, upper=K> idx;
}
parameters {
  real theta;
  matrix[K, K] x;
}
model {
  int done = 0;
  while (theta > done) {
    target += gathered_trace(x[idx, idx]) * theta;
    done += 1;
  }
  target += theta;
}
