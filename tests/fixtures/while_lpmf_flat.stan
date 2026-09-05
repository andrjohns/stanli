data {
  int<lower=0> N;
  array[N] int y;
  int<lower=0> reps;
}
parameters {
  real eta;
}
model {
  real acc = 0;
  for (k in 0 : reps - 1) {
    acc += poisson_log_lpmf(y[1] | eta + k) * log2();
  }
  target += acc;
  target += normal_lpdf(eta | 0, 1);
}
