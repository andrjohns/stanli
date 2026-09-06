functions {
  vector rhs(real t, vector y, real k) {
    matrix[1100, 1000] big = rep_matrix(k, 1100, 1000);
    return -big[1, 1] * y;
  }
}
transformed data {
  array[3] real ts = {0.5, 1.0, 1.5};
}
parameters {
  real<lower=0> k;
}
transformed parameters {
  array[3] vector[1] sol = ode_rk45(rhs, [1.0]', 0.0, ts, k);
}
model {
  k ~ lognormal(0, 1);
}
