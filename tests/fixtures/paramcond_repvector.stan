// rep_vector inside a parameter-dependent region. The register file is a
// flat run of doubles, so the repeated value is a run the compiler fills;
// the extent is a compile-time integer like every other extent in a
// region. A repeated parameter carries the broadcast's adjoint: every
// element's adjoint adds into the one cell the value came from.
data {
  int n;
}
parameters {
  real theta;
}
model {
  if (theta > 0) {
    vector[n] repeated = rep_vector(theta, n);
    vector[n] constant = rep_vector(0.5, n);
    target += sum(repeated) + sum(constant);
  } else {
    target += theta;
  }
}
