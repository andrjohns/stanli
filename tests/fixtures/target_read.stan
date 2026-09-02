functions {
  real snapshot_lp(real shift) {
    return target() + shift;
  }
}

parameters {
  real x;
}

model {
  target += -0.5 * square(x);
  target += 0.25 * target();
  if (x > 0) {
    target += 2;
    target += snapshot_lp(3);
  }
}
