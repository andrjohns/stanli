// normal_id_glm with a parameter design matrix X.
data {
  int<lower=1> N;
  int<lower=1> K;
  vector[N] y;
}
parameters {
  matrix[N, K] X;
  vector[K] beta;
  real alpha;
  real<lower=0> sigma;
}
model {
  target += normal_id_glm_lpdf(y | X, alpha, beta, sigma);
}
