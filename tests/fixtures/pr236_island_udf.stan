functions {
  real pick(matrix x, array[] int rows, array[] int columns) {
    return sum(x[rows, columns]);
  }
}
parameters {
  matrix[3, 3] m;
}
model {
  if (m[1, 1] > 0)
    target += pick(m, {3, 1}, {2, 3});
}
