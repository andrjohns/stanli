// generated with brms 2.23.0
functions {
  /* multi-normal log-PDF for fixed correlation matrices
   * assuming homogoneous variances
   * Args:
   *   y: response vector
   *   mu: mean parameter vector
   *   sigma: residual standard deviation
   *   chol_cor: cholesky factor of the correlation matrix
   * Returns:
   *   sum of the log-PDF values of all observations
   */
  real normal_fcor_hom_lpdf(vector y, vector mu, real sigma, data matrix chol_cor) {
    return multi_normal_cholesky_lpdf(y | mu, sigma * chol_cor);
  }
  /* multi-normal log-PDF for fixed correlation matrices
   * assuming heterogenous variances
   * Args:
   *   y: response vector
   *   mu: mean parameter vector
   *   sigma: residual standard deviation vector
   *   chol_cor: cholesky factor of the correlation matrix
   * Returns:
   *   sum of the log-PDF values of all observations
   */
  real normal_fcor_het_lpdf(vector y, vector mu, vector sigma, data matrix chol_cor) {
    return multi_normal_cholesky_lpdf(y | mu, diag_pre_multiply(sigma, chol_cor));
  }
}
data {
  int<lower=1> N;  // total number of observations
  vector[N] Y;  // response variable
  int<lower=1> K;  // number of population-level effects
  matrix[N, K] X;  // population-level design matrix
  int<lower=1> Kc;  // number of population-level effects after centering
  matrix[N, N] Mfcor;  // known residual covariance matrix
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
  matrix[N, Kc] Xc;  // centered version of X without an intercept
  vector[Kc] means_X;  // column means of X before centering
  matrix[N, N] Lfcor = cholesky_decompose(Mfcor);
  for (i in 2:K) {
    means_X[i - 1] = mean(X[, i]);
    Xc[, i - 1] = X[, i] - means_X[i - 1];
  }
}
parameters {
  vector[Kc] b;  // regression coefficients
  real Intercept;  // temporary intercept for centered predictors
  real<lower=0> sigma;  // dispersion parameter
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept | 3, -0.3, 2.5);
  lprior += student_t_lpdf(sigma | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] mu = rep_vector(0.0, N);
    mu += Intercept + Xc * b;
    target += normal_fcor_hom_lpdf(Y | mu, sigma, Lfcor);
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_Intercept = Intercept - dot_product(means_X, b);
}

