// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
// STANLI-LIT-DATA: {"lb": -Infinity, "ub": 3.5, "N": 3, "y": [1.5, Infinity, NaN]}
data {
  real lb;
  real<upper=5> ub;
  int N;
  vector[N] y;
}
parameters { real mu; }
model {
  mu ~ normal(0, 1);
  target += ub + y[1];
}
