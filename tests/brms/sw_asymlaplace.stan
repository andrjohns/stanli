// generated with brms 2.23.0
functions {
  /* helper function for asym_laplace_lpdf
   * Args:
   *   y: the response value
   *   quantile: quantile parameter in (0, 1)
   */
   real rho_quantile(real y, real quantile) {
     if (y < 0) {
       return y * (quantile - 1);
     } else {
       return y * quantile;
     }
   }
  /* asymmetric laplace log-PDF for a single response
   * Args:
   *   y: the response value
   *   mu: location parameter
   *   sigma: positive scale parameter
   *   quantile: quantile parameter in (0, 1)
   * Returns:
   *   a scalar to be added to the log posterior
   */
   real asym_laplace_lpdf(real y, real mu, real sigma, real quantile) {
     return log(quantile * (1 - quantile)) -
            log(sigma) -
            rho_quantile((y - mu) / sigma, quantile);
   }
  /* asymmetric laplace log-CDF for a single quantile
   * Args:
   *   y: a quantile
   *   mu: location parameter
   *   sigma: positive scale parameter
   *   quantile: quantile parameter in (0, 1)
   * Returns:
   *   a scalar to be added to the log posterior
   */
   real asym_laplace_lcdf(real y, real mu, real sigma, real quantile) {
     if (y < mu) {
       return log(quantile) + (1 - quantile) * (y - mu) / sigma;
     } else {
       return log1m((1 - quantile) * exp(-quantile * (y - mu) / sigma));
     }
   }
  /* asymmetric laplace log-CCDF for a single quantile
   * Args:
   *   y: a quantile
   *   mu: location parameter
   *   sigma: positive scale parameter
   *   quantile: quantile parameter in (0, 1)
   * Returns:
   *   a scalar to be added to the log posterior
   */
   real asym_laplace_lccdf(real y, real mu, real sigma, real quantile) {
     if (y < mu) {
       return log1m(quantile * exp((1 - quantile) * (y - mu) / sigma));
     } else {
       return log1m(quantile) - quantile * (y - mu) / sigma;
     }
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
  real<lower=0> sigma;  // dispersion parameter
  real<lower=0,upper=1> quantile;  // quantile parameter
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept | 3, 0.2, 2.5);
  lprior += student_t_lpdf(sigma | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += beta_lpdf(quantile | 1, 1);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] mu = rep_vector(0.0, N);
    mu += Intercept + Xc * b;
    for (n in 1:N) {
      target += asym_laplace_lpdf(Y[n] | mu[n], sigma, quantile);
    }
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_Intercept = Intercept - dot_product(means_X, b);
}

