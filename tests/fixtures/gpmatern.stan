parameters {
  array[3] vector[2] x;
  real<lower=0> alpha;
  real<lower=0> rho;
}
model {
  matrix[3, 3] a = gp_matern32_cov(x, alpha, rho);
  matrix[3, 3] b = gp_matern52_cov(x, alpha, rho);
  matrix[3, 3] c = gp_exponential_cov(x, alpha, rho);
  target += sum(a) + 2 * sum(b) + 3 * sum(c);
  target += a[1, 2] * b[2, 3] * c[1, 3];
}
generated quantities {
  matrix[3, 3] Kq = gp_exp_quad_cov(x, alpha, rho);
  matrix[3, 3] K32 = gp_matern32_cov(x, alpha, rho);
  matrix[3, 3] K52 = gp_matern52_cov(x, alpha, rho);
  matrix[3, 3] Kexp = gp_exponential_cov(x, alpha, rho);
}
