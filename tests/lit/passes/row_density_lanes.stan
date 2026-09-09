// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
// STANLI-LIT-DATA: {"R": 4, "C": 3, "x": [[0.1, 0.2, 0.3], [0.5, 0.6, 0.7], [0.9, 1.0, 1.1], [1.3, 1.4, 1.5]], "y": [[0, 1, 0], [1, 1, 0], [0, 0, 1], [1, 0, 1]]}
// STANLI-LIT-DUMP: log_prob:reroll
// STANLI-LIT-CHECK-NOT: SET_SLICE_STRIDED
// STANLI-LIT-CHECK: BERNOULLI_LOGIT_LPMF
// STANLI-LIT-CHECK-NOT: BERNOULLI_LOGIT_LPMF
data {
  int R;
  int C;
  matrix[R, C] x;
  array[R, C] int y;
}
parameters {
  vector[3] b;
}
transformed parameters {
  matrix[R, C] p;
  for (j in 1 : R) {
    p[j, :] = fma(b[3], x[j, :], b[1]);
  }
}
model {
  b ~ normal(0, 10);
  for (i in 1 : R) {
    target += bernoulli_logit_lupmf(y[i] | p[i]);
  }
}
