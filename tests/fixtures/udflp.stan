// Model-block user-defined functions on parameters, inlined at lowering:
// scalar expression body, vector return, statement body with a local
// accumulator and loop.
functions {
  real affine(real a, real b, real x) {
    return a * x + b;
  }
  vector halfv(vector v) {
    return v / 2;
  }
  real accum(vector v) {
    real s = 0;
    for (i in 1 : rows(v)) {
      s = s + v[i];
    }
    return s / 2;
  }
}
data {
  int<lower=1> N;
  vector[N] y;
}
parameters {
  real mu;
  vector[N] d;
}
model {
  target += normal_lpdf(y | affine(mu, 0.5, 2.0), 1);
  target += normal_lpdf(halfv(d) | 0, 1);
  target += normal_lpdf(accum(d) | 0, 1);
}
