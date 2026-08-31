// The callback resolver must distinguish overloads, preserve active slices,
// and give nested reductions independent bindings and outer-array bounds.
functions {
  real partial(array[] real s, int start, int end, real b) {
    return b * sum(s);
  }
  real partial(array[] int s, int start, int end, real b) {
    return b * sum(s);
  }
  real nested(array[,] real s, int start, int end, real b) {
    real out = 0;
    for (i in 1:size(s)) out += reduce_sum(partial, s[i], 1, b);
    return out;
  }
}
parameters {
  array[3] real z;
  real b;
}
model {
  array[3] int counts = {1, 2, 3};
  target += reduce_sum(partial, z, 2, b);
  target += reduce_sum_static(partial, counts, 100, b);
  target += reduce_sum(nested, {z, z}, 1, b);
}
