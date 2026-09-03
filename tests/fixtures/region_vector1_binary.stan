parameters {
  real x;
}

model {
  target += -0.5 * square(x);
  if (x > 0) {
    vector[1] v = [x]';
    target += hypot(v, 3.0)[1];
  }
}
