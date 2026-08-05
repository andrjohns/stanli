// Data-only expressions that have no native lowering fold to constants at
// compile time: statistic bounds (mean/sd), negative_infinity inside
// log_sum_exp, and an all-constant lccdf term.
data {
  int<lower=0> N;
  vector[N] y;
}
parameters {
  vector<lower=mean(y) - 3 * sd(y), upper=mean(y) + 3 * sd(y)>[2] mu;
}
model {
  target += normal_lpdf(y[1] | mu[1], 1);
  target += log_sum_exp(normal_lpdf(y[2] | mu[2], 1), negative_infinity());
  target += student_t_lccdf(0 | 3, 0, 10);
}
