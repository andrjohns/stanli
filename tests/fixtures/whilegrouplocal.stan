// brms writes normal_time_hom_flex_lpdf for unstr() autocorrelation as a
// while over groups whose body declares array[nobs[i]] int and calls a
// helper with an early return. Every condition is data, so the loop has a
// compile-time trip count and the extents fold with it.
functions {
  int match_len(array[] int a, int k) {
    int n = size(a);
    if (n != k) {
      return 0;
    }
    return 1;
  }
  real groups_lpdf(vector y, array[] int nobs, array[,] int J) {
    real lp = 0;
    int I = size(nobs);
    int i = 1;
    while (i <= I) {
      array[nobs[i]] int idx = J[i, 1 : nobs[i]];
      if (match_len(idx, 3) == 1) {
        lp += sum(y[idx]) * nobs[i];
      } else {
        lp += prod(y[idx]);
      }
      i += 1;
    }
    return lp;
  }
}
data {
  int<lower=1> N;
  array[3] int<lower=1> nobs;
  array[3, 3] int<lower=1> J;
}
parameters {
  vector[N] y;
}
model {
  target += groups_lpdf(y | nobs, J);
  y ~ std_normal();
}
