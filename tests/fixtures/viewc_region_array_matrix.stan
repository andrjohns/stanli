// A break makes the loop a runtime-control region, so the register program
// compiles the array-of-matrix local. The read positions differ under a
// row-major leaf, which pins the column-major layout.
parameters {
  real q;
}
model {
  real r = 0;
  for (i in 1 : 1) {
    array[2] matrix[2, 3] x = {
      [[1, 2, 3], [4, 5, 6]],
      [[7, 8, 9], [10, 11, 12]]
    };
    if (x[1][2, 3] == 6 && x[2][1, 2] == 8) {
      r = x[2][2, 1] * q;
      break;
    }
    r = 100 * q;
  }
  target += r;
}
