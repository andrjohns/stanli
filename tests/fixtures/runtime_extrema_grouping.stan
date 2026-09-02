data {
  int mode;
}
parameters {
  real probe;
  matrix[2, 2] m;
}
model {
  if (probe > 0) {
    if (mode == 1)
      target += 1 / min(m[1]);
    else
      target += 1 / max(m[1]);
  }
}
