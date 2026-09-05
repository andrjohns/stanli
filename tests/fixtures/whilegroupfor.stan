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
    for (i in 1 : size(nobs)) {
      if (match_len(J[i, 1 : nobs[i]], 3) == 1) {
        lp += sum(y[J[i, 1 : nobs[i]]]) * nobs[i];
      } else {
        lp += prod(y[J[i, 1 : nobs[i]]]);
      }
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
