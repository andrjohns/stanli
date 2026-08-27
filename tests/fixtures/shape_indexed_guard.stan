functions {
  real indexed_rows(matrix mat, array[] int idx) {
    if (rows(mat[idx, idx]) == 0) return 1.0;
    if (rows(mat[1, idx]) != 1 || cols(mat[1, idx]) != size(idx))
      return -10.0;
    if (rows(mat[idx, 1]) != size(idx) || cols(mat[idx, 1]) != 1)
      return -20.0;
    return size(idx);
  }
}
data {
  int<lower=0> K;
  array[K] int idx;
}
parameters {
  matrix[3, 3] M;
  real theta;
}
model {
  target += indexed_rows(M, idx) * theta;
}
