functions {
  matrix shape_design(matrix mat) {
    if (rows(mat) == 0) return rep_matrix(1.0, 1, 1);
    return rep_matrix(2.0, 1, 1);
  }
}
data {
  int<lower=0> n;
  vector[1] y;
}
parameters {
  matrix[n, 2] probe;
  real alpha;
  vector[1] beta;
}
model {
  y ~ normal_id_glm(shape_design(probe), alpha, beta, 1);
}
