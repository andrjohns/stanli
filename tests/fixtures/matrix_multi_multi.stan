// Two integer arrays select the Cartesian submatrix, retaining their order in
// each axis rather than pairing corresponding indices.
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
}
