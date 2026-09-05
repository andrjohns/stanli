data {
  real e1;
}
parameters {
  real a;
  real b;
}
model {
  target += pow(a, 1);
  target += pow(a, e1);
  target += pow(a, 2);
  target += pow(a, b);
  for (i in 1 : 3) {
    target += pow(a, i);
  }
}
