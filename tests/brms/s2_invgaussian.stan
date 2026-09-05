// generated with brms 2.23.0
functions {
  /* inverse Gaussian log-PDF for a single response
   * Args:
   *   y: the response value
   *   mu: positive mean parameter
   *   shape: positive shape parameter
   * Returns:
   *   a scalar to be added to the log posterior
   */
   real inv_gaussian_lpdf(real y, real mu, real shape) {
     return 0.5 * log(shape / (2 * pi())) -
            1.5 * log(y) -
            0.5 * shape * square((y - mu) / (mu * sqrt(y)));
   }
  /* vectorized inverse Gaussian log-PDF
   * Args:
   *   y: response vector
   *   mu: positive mean parameter vector
   *   shape: positive shape parameter
   * Returns:
   *   a scalar to be added to the log posterior
   */
   real inv_gaussian_lpdf(vector y, vector mu, real shape) {
     return 0.5 * rows(y) * log(shape / (2 * pi())) -
            1.5 * sum(log(y)) -
            0.5 * shape * dot_self((y - mu) ./ (mu .* sqrt(y)));
   }
  /* inverse Gaussian log-CDF for a single quantile
   * Args:
   *   y: a quantile
   *   mu: positive mean parameter
   *   shape: positive shape parameter
   * Returns:
   *   log(P(Y <= y))
   */
   real inv_gaussian_lcdf(real y, real mu, real shape) {
     return log(Phi(sqrt(shape) / sqrt(y) * (y / mu - 1)) +
              exp(2 * shape / mu) * Phi(-sqrt(shape) / sqrt(y) * (y / mu + 1)));
   }
  /* inverse Gaussian log-CCDF for a single quantile
   * Args:
   *   y: a quantile
   *   mu: positive mean parameter
   *   shape: positive shape parameter
   * Returns:
   *   log(P(Y > y))
   */
   real inv_gaussian_lccdf(real y, real mu, real shape) {
     return log1m(Phi(sqrt(shape) / sqrt(y) * (y / mu - 1)) -
              exp(2 * shape / mu) * Phi(-sqrt(shape) / sqrt(y) * (y / mu + 1)));
   }
}
data {
  int<lower=1> N;  // total number of observations
  vector[N] Y;  // response variable
  int<lower=1> K;  // number of population-level effects
  matrix[N, K] X;  // population-level design matrix
  int<lower=1> Kc;  // number of population-level effects after centering
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
  matrix[N, Kc] Xc;  // centered version of X without an intercept
  vector[Kc] means_X;  // column means of X before centering
  for (i in 2:K) {
    means_X[i - 1] = mean(X[, i]);
    Xc[, i - 1] = X[, i] - means_X[i - 1];
  }
}
parameters {
  vector[Kc] b;  // regression coefficients
  real Intercept;  // temporary intercept for centered predictors
  real<lower=0> shape;  // shape parameter
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept | 3, 2.5, 3.3);
  lprior += gamma_lpdf(shape | 0.01, 0.01);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] mu = rep_vector(0.0, N);
    mu += Intercept + Xc * b;
    mu = inv_sqrt(mu);
    target += inv_gaussian_lpdf(Y | mu, shape);
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_Intercept = Intercept - dot_product(means_X, b);
}

