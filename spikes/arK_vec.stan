data {
  int<lower=0> K;
  int<lower=0> T;
  array[T] real y;
}
transformed data {
  int M = T - K;
  vector[M] yv;
  matrix[M, K] Ylag;
  for (i in 1 : M) {
    yv[i] = y[K + i];
    for (k in 1 : K) {
      Ylag[i, k] = y[K + i - k];
    }
  }
}
parameters {
  real alpha;
  array[K] real beta;
  real<lower=0> sigma;
}
model {
  alpha ~ normal(0, 10);
  beta ~ normal(0, 10);
  sigma ~ cauchy(0, 2.5);

  // re-rolled form of the original's per-t loop: mu[t] = alpha + sum_k
  // beta[k] * y[t-k] becomes one matvec plus a broadcast add.
  yv ~ normal(alpha + Ylag * to_vector(beta), sigma);
}
