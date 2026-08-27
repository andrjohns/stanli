// O1 inlining implements an early UDF return with a single-iteration loop
// and Break statements. The runtime must retain and execute that control
// flow even though the source contains no explicit break.
functions {
  real choose(real x, int first) {
    if (first) return 2 * x;
    return 3 * x;
  }
}
data {
  int first;
}
parameters {
  real theta;
}
model {
  target += choose(theta, first);
}
