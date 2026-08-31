data {
  int<lower=1> N;
}
transformed data {
  vector[N] zv = zeros_vector(N);
  row_vector[N] orv = ones_row_vector(N);
  array[N] int zia = zeros_int_array(N);
  matrix[N, N] id = identity_matrix(N);
  real s = sum(zv) + sum(orv) + zia[1] + id[1, 1] + id[2, 2] + id[N, N];
}
parameters {
  vector[N] x;
}
model {
  vector[N] ov = ones_vector(N);
  row_vector[N] zrv = zeros_row_vector(N);
  target += dot_product(x, ov) + zrv * x + s;
}
