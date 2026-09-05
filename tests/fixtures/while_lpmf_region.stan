functions {
  real accumulate_lpmf(int y, real eta, real gate) {
    real acc = 0;
    real term = gate + 1.0;
    int k = 0;
    while (term > 0.5) {
      acc += poisson_log_lpmf(y | eta + k) * log2();
      term -= 0.5;
      k += 1;
    }
    return acc;
  }
}
data {
  int<lower=0> N;
  array[N] int y;
}
parameters {
  real eta;
}
model {
  target += accumulate_lpmf(y[1] | eta, inv_logit(eta));
  target += normal_lpdf(eta | 0, 1);
}
