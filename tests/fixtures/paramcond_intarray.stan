// Integer reads that only the caller can answer, in the compile-time
// integer positions inside a parameter-dependent region: an element of a
// data array at rank two, and a literal array's size and elements --
// which is the form stanc's inliner leaves behind when it substitutes an
// `array[] int` argument at a call site.
data {
  array[3, 2] int idx;
}
parameters {
  real theta;
}
model {
  if (theta > 0) {
    int total = 0;
    for (r in 1 : 3) {
      if (idx[r, 2] == 5) total += idx[r, 1];
    }
    int m = size({4, 5, 6}) + {4, 5, 6}[2];
    target += (total + m) * theta;
  } else {
    target += theta;
  }
}
