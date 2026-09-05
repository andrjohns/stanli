data {
  real d;
  vector[3] dv;
}
transformed data {
  real td = 2 - d;
}
parameters {
  real a;
}
model {
  real q = 2 - d;
  target += pow(a, q);
  target += pow(a, td);
  for (i in 1 : 3) {
    real r = dv[i] * 1.0;
    target += pow(a, r);
  }
}
