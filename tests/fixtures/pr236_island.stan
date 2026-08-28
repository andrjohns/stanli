functions {
  real selected_cell(matrix x) {
    return x[1, 2];
  }
}
parameters {
  matrix[3, 3] m;
}
model {
  matrix[3, 3] x = 2 * m;
  if (m[1, 1] > 0) {
    target += sum(x[{3, 1}, {2, 3}]);
    target += selected_cell(x[{3, 1}, {2, 3}]);
    for (i in {2, 3})
      target += i * m[1, 1];
  } else {
    target += m[1, 1];
  }
}
