// Two integer arrays select a Cartesian submatrix, retaining their order in
// each axis rather than pairing corresponding indices. Ranges and mixed
// row/column selectors exercise the same two-axis selection path.
data {
  array[2] int row_indices;
  array[2] int column_indices;
}
parameters {
  matrix[3, 3] m;
}
model {
  matrix[3, 3] computed = 2.0 * m;
  target += sum(computed[row_indices, column_indices]);
  target += sum(computed[1:2, 2:3]);
  target += sum(computed[row_indices, 1]);
  target += sum(computed[1, column_indices]);
}
