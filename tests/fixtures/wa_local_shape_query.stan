functions {
  real local_extents(real x) {
    vector[3] v = rep_vector(x, 3);
    int n = num_elements(v);
    matrix[2, 4] m = rep_matrix(x, 2, 4);
    int r = rows(m);
    int c = cols(m);
    return n + 10 * r + 100 * c + x;
  }
}
parameters {
  real theta;
}
model {
  target += local_extents(theta);
}
generated quantities {
  real q = local_extents(theta);
}
