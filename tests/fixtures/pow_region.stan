data {
  real d;
}
parameters {
  real a;
}
model {
  if (a > -1) {
    target += pow(a, 1);
    target += pow(a, 2);
    target += pow(a, d);
  } else {
    target += 0;
  }
}
