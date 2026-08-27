// A break selected by a parameter must be compiled with its enclosing loop,
// otherwise a conditional island has no jump target for the break.
parameters {
  real theta;
}
model {
  real contribution = 0;
  for (i in 1:3) {
    if (theta > 0) break;
    contribution += theta;
  }
  target += contribution;
}
