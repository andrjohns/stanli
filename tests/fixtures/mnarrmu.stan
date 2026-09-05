data {
  int N;
  int K;
  array[N] vector[K] y;
  vector[K] y1;
  matrix[K, K] L;
}
parameters {
  array[N] vector[K] Mu;
  real<lower=0> s;
}
model {
  matrix[K, K] Ls = s * L;
  target += multi_normal_cholesky_lpdf(y | Mu, Ls);
  target += multi_normal_cholesky_lpdf(y1 | Mu, Ls);
}
