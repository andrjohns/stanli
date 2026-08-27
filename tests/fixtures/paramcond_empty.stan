// A parameter-dependent region whose live-outs are all zero-width. The
// model's dimension table gives these matrices no extent, so the region
// has nothing to carry out: there is no island to run and the values keep
// the empty shape they already have. That is not the same as a region
// that found no live-out at all, which is the case that still refuses.
data {
  int n;
}
parameters {
  real theta;
}
model {
  matrix[n, n] source;
  matrix[n, n] out;
  if (theta > 0) {
    out = source;
  }
  target += theta;
}
