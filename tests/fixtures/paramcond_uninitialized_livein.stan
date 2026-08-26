// A parameter-dependent region reads a shaped local before its first write.
// Stan materializes the matrix with NaNs; the region must bind that value as
// a live-in rather than treating the declared name as unknown.
parameters {
  real theta;
}
transformed parameters {
  matrix[1, 1] uninitialized;
  real selected = 0;
  if (theta > 0) selected = uninitialized[1, 1];
}
model {
  theta ~ std_normal();
  target += selected;
}
