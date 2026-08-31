data {
  matrix[3, 4] A;
}
transformed data {
  array[4] int u = csr_extract_u(A);
  array[5] int v = csr_extract_v(A);
  vector[5] w = csr_extract_w(A);
  int usum = u[1] + u[2] + u[3] + u[4];
  int vsum = v[1] + v[2] + v[3] + v[4] + v[5];
}
parameters {
  vector[5] x;
}
model {
  target += dot_product(x, w) + usum + vsum;
}
