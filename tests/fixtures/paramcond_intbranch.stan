// The same integer, assigned where the assignment may not run. Folding it
// would apply to every later read, including on the path that skipped it,
// and no register can carry an integer back to the lowering instead --
// which held its own pre-region copy. Refused, by name.
parameters {
  real theta;
}
model {
  int n = 1;
  real z = 0;
  if (theta > 0) {
    n = 2;
    z = theta * 5;
  }
  target += n * theta + z;
}
