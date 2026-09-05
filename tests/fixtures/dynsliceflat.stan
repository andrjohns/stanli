functions {
  real acc_lpdf(vector y, array[] int nobs, int op) {
    real lp = 0;
    for (i in 1 : size(nobs)) {
      int k = nobs[i];
      vector[3] t;
      vector[3] u;
      t[1] = y[i];
      t[2] = y[i] * 2;
      t[3] = y[i] * 3;
      u[1] = y[i] + 1;
      u[2] = y[i] - 1;
      u[3] = y[i] * y[i];
      if (op == 1) {
        lp += log_sum_exp(t[1 : k]);
      }
      if (op == 2) {
        lp += max(t[1 : k]) + min(t[1 : k]);
      }
      if (op == 3) {
        lp += num_elements(t[1 : k]) * y[i] + size(u[1 : k]);
      }
      if (op == 4) {
        lp += sum(t[1 : k]) + prod(t[1 : k]);
      }
      if (op == 5) {
        lp += normal_lpdf(t[1 : k] | y[i], 1);
      }
      if (op == 6) {
        lp += sum(t[1 : k] .* u[1 : k]);
      }
      if (op == 7) {
        lp += dot_product(t[1 : k], u[1 : k]) + dot_self(t[1 : k]);
      }
      if (op == 8) {
        lp += mean(t[1 : k]) + sd(t[1 : k]) + variance(t[1 : k]);
      }
      if (op == 9) {
        lp += log_sum_exp(exp(t[1 : k]));
      }
    }
    return lp;
  }
}
data {
  int<lower=1> N;
  array[3] int<lower=1, upper=3> nobs;
  int op;
}
parameters {
  vector[N] y;
}
model {
  target += acc_lpdf(y | nobs, op);
  y ~ std_normal();
}
