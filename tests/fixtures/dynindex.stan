functions {
  real acc_lpdf(vector y, matrix a, array[] int nobs, int op) {
    real lp = 0;
    int I = size(nobs);
    int i = 1;
    while (i <= I) {
      int k = nobs[i];
      vector[3] t;
      vector[3] u;
      array[3] int idx;
      array[3] vector[2] arr;
      t[1] = y[i];
      t[2] = y[i] * 2;
      t[3] = y[i] * 3;
      u[1] = y[i] + 1;
      u[2] = y[i] - 1;
      u[3] = y[i] * y[i];
      idx[1] = 2;
      idx[2] = 1;
      idx[3] = 3;
      arr[1] = [t[1], u[1]]';
      arr[2] = [t[2], u[2]]';
      arr[3] = [t[3], u[3]]';
      if (op == 1) {
        lp += sum(y[idx[1 : k]]);
      }
      if (op == 2) {
        lp += (rows(a[1 : k, 1 : k]) + cols(a[1 : k, 1 : k])) * y[i];
      }
      if (op == 3) {
        lp += size(arr[1 : k]) * y[i];
      }
      if (op == 4) {
        lp += sum(t[1 : k] .* u);
      }
      if (op == 5) {
        lp += sum(u .* t[1 : k]);
      }
      if (op == 6) {
        lp += dot_product(t[1 : k], u);
      }
      if (op == 7) {
        lp += num_elements(arr[1 : k]) * y[i];
      }
      i += 1;
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
  matrix[3, 3] a;
}
transformed parameters {
  real s = acc_lpdf(y | a, nobs, op);
}
model {
  target += s;
  y ~ std_normal();
  to_vector(a) ~ std_normal();
}
