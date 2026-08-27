functions {
  real lazy_rows(matrix mat, array[] int empty, array[] int valid,
                 array[] int bad) {
    real out = 0.0;
    if (rows(mat[empty, empty]) == 0 || rows(mat[bad, bad]) == 0)
      out += 1.0;
    if (rows(mat[valid, valid]) == 0 && rows(mat[bad, bad]) == 0)
      out += 10.0;
    if (rows(mat[empty, empty]) == 0)
      out += 0.0;
    else if (rows(mat[bad, bad]) == 0)
      out += 100.0;
    out += rows(mat[empty, empty]) == 0
             ? 0.0
             : (rows(mat[bad, bad]) == 0 ? 1000.0 : 2000.0);
    return out;
  }
}
data {
  array[0] int empty;
  array[1] int valid;
  array[1] int bad;
}
parameters {
  matrix[2, 2] M;
  real theta;
}
model {
  target += lazy_rows(M, empty, valid, bad) * theta;
}
