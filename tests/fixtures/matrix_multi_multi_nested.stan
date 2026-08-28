// O1 substitutes the gathered matrix argument into the UDF's scalar index,
// leaving an empty outer Indexed node that carries the final scalar type.
functions {
  real selected_cell(matrix x) {
    return x[1, 2];
  }
}
data {
  array[2] int row_indices;
  array[2] int column_indices;
}
parameters {
  matrix[3, 3] m;
}
model {
  matrix[3, 3] computed = 2.0 * m;
  target += selected_cell(computed[row_indices, column_indices]);
}
