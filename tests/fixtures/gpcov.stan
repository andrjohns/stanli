parameters {
  array[3] real x;
  real<lower=0> alpha;
  real<lower=0> rho;
}
model {
  matrix[3, 3] K = gp_exp_quad_cov(x, alpha, rho);
  target += sum(K);
  target += K[1, 2] * K[2, 3];
}
