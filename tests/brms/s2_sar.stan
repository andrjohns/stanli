// generated with brms 2.23.0
functions {
  /* normal log-pdf for spatially lagged responses
   * Args:
   *   y: the response vector
   *   mu: mean parameter vector
   *   sigma: residual standard deviation
   *   rho: positive autoregressive parameter
   *   W: spatial weight matrix
   *   eigenW: precomputed eigenvalues of W
   * Returns:
   *   a scalar to be added to the log posterior
   */
  real normal_lagsar_lpdf(vector y, vector mu, real sigma,
                          real rho, data matrix W, data vector eigenW) {
    int N = rows(y);
    real inv_sigma2 = inv_square(sigma);
    matrix[N, N] W_tilde = add_diag(-rho * W, 1);
    vector[N] half_pred;
    real log_det;
    half_pred = W_tilde * y - mu;
    log_det = sum(log1m(rho * eigenW));
    return  0.5 * N * log(inv_sigma2) + log_det -
      0.5 * dot_self(half_pred) * inv_sigma2;
  }
}
data {
  int<lower=1> N;  // total number of observations
  vector[N] Y;  // response variable
  int<lower=1> K;  // number of population-level effects
  matrix[N, K] X;  // population-level design matrix
  int<lower=1> Kc;  // number of population-level effects after centering
  matrix[N, N] Msar;  // spatial weight matrix
  vector[N] eigenMsar;  // eigenvalues of Msar
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
  matrix[N, Kc] Xc;  // centered version of X without an intercept
  vector[Kc] means_X;  // column means of X before centering
  // the eigenvalues define the boundaries of the SAR correlation
  real min_eigenMsar = min(eigenMsar);
  real max_eigenMsar = max(eigenMsar);
  for (i in 2:K) {
    means_X[i - 1] = mean(X[, i]);
    Xc[, i - 1] = X[, i] - means_X[i - 1];
  }
}
parameters {
  vector[Kc] b;  // regression coefficients
  real Intercept;  // temporary intercept for centered predictors
  real<lower=min_eigenMsar,upper=max_eigenMsar> lagsar;  // lag-SAR correlation parameter
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
    target += normal_lagsar_lpdf(Y | mu, sigma, lagsar, Msar, eigenMsar);
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_Intercept = Intercept - dot_product(means_X, b);
}

