functions {
  real observed_plus_active(vector observed, real active) {
    // Keep the call visible in transformed MIR. The first actual is a large
    // observed container while the second makes the call itself non-foldable.
    array[2] matrix[2, 2] uninlined_shape_guard;
    return observed[1] + active;
  }
}
data {
  int<lower=2> N;
  vector[N] x;
}
parameters {
  real theta;
}
model {
  target += observed_plus_active(x, theta);
}
