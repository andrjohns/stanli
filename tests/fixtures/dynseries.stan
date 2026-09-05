functions {
  real series(real theta) {
    int M = 8;
    int k = 2;
    vector[M] t;
    if (theta == 1) return -1;
    t[1] = -theta;
    t[2] = -2 * theta;
    while (t[k] >= t[k - 1] - 1 && k < M) {
      k += 1;
      t[k] = -k * theta;
    }
    return log_sum_exp(t[1 : k]);
  }
}
data {
  real theta;
}
parameters {
  real z;
}
model {
  target += series(theta);
  target += 0 * z;
}
