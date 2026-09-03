functions {
  real scaled_partial(array[] real slice, int start, int end, real scale) {
    return scale * sum(slice);
  }
}

parameters {
  real x;
}

model {
  target += -0.5 * square(x);
  if (x > 0) {
    target += reduce_sum(scaled_partial, {x, 2 * x}, 1, x);
  }
}
