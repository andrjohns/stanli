data {
  int<lower=1> T;
  int<lower=1> K;
  array[T] real y;
}
parameters {
  simplex[K] pi1;
  array[K] simplex[K] A;
  ordered[K] mu;
  array[K] real<lower=0> sigma;
}
transformed parameters {
  array[T] vector[K] logalpha;
  {
    array[K] real accumulator;
    logalpha[1] = log(pi1) + normal_lpdf(y[1] | mu, sigma);
    for (t in 2 : T) {
      for (j in 1 : K) {
        for (i in 1 : K) {
          accumulator[i] = logalpha[t - 1, i] + log(A[i, j])
                           + normal_lpdf(y[t] | mu[j], sigma[j]);
        }
        logalpha[t, j] = log_sum_exp(accumulator);
      }
    }
  }
}
model {
  target += log_sum_exp(logalpha[T]);
}
