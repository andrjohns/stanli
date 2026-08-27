// A declared but uninitialized local read only inside a parameter-dependent
// branch must be a NaN-filled island live-in, not an unknown variable.
parameters {
  real theta;
}
model {
  matrix[1, 1] uninitialized;
  real contribution;
  if (theta > 0) {
    contribution = uninitialized[1, 1];
  } else {
    contribution = theta;
  }
  target += contribution;
}
