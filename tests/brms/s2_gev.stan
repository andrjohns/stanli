// generated with brms 2.23.0
functions {
  /* generalized extreme value log-PDF for a single response
   * Args:
   *   y: the response value
   *   mu: location parameter
   *   sigma: scale parameter
   *   xi: shape parameter
   * Returns:
   *   a scalar to be added to the log posterior
   */
   real gen_extreme_value_lpdf(real y, real mu, real sigma, real xi) {
     real x = (y - mu) / sigma;
     if (xi == 0) {
       return - log(sigma) - x - exp(-x);
     } else {
       real t = 1 + xi * x;
       real inv_xi = 1 / xi;
       return - log(sigma) - (1 + inv_xi) * log(t) - pow(t, -inv_xi);
     }
   }
  /* generalized extreme value log-CDF for a single response
   * Args:
   *   y: a quantile
   *   mu: location parameter
   *   sigma: scale parameter
   *   xi: shape parameter
   * Returns:
   *   log(P(Y <= y))
   */
   real gen_extreme_value_lcdf(real y, real mu, real sigma, real xi) {
     real x = (y - mu) / sigma;
     if (xi == 0) {
       return - exp(-x);
     } else {
       return - pow(1 + xi * x, - 1 / xi);
     }
   }
  /* generalized extreme value log-CCDF for a single response
   * Args:
   *   y: a quantile
   *   mu: location parameter
   *   sigma: scale parameter
   *   xi: shape parameter
   * Returns:
   *   log(P(Y > y))
   */
   real gen_extreme_value_lccdf(real y, real mu, real sigma, real xi) {
     return log1m_exp(gen_extreme_value_lcdf(y | mu, sigma, xi));
   }
  /* scale auxiliary parameter xi to a suitable region
   * expecting sigma to be a scalar
   * Args:
   *   xi: unscaled shape parameter
   *   y: response values
   *   mu: location parameter
   *   sigma: scale parameter
   * Returns:
   *   scaled shape parameter xi
   */
  real scale_xi(real xi, vector y, vector mu, real sigma) {
    vector[rows(y)] x = (y - mu) / sigma;
    vector[2] bounds = [-inv(min(x)), -inv(max(x))]';
    real lb = min(bounds);
    real ub = max(bounds);
    return inv_logit(xi) * (ub - lb) + lb;
  }
  /* scale auxiliary parameter xi to a suitable region
   * expecting sigma to be a vector
   * Args:
   *   xi: unscaled shape parameter
   *   y: response values
   *   mu: location parameter
   *   sigma: scale parameter
   * Returns:
   *   scaled shape parameter xi
   */
  real scale_xi(real xi, vector y, vector mu, vector sigma) {
    vector[rows(y)] x = (y - mu) ./ sigma;
    vector[2] bounds = [-inv(min(x)), -inv(max(x))]';
    real lb = min(bounds);
    real ub = max(bounds);
    return inv_logit(xi) * (ub - lb) + lb;
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
  real tmp_xi;  // shape parameter (temporary)
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept | 3, -0.3, 2.5);
  lprior += student_t_lpdf(sigma | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += normal_lpdf(tmp_xi | 0, 2.5);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] mu = rep_vector(0.0, N);
    real xi;  // scaled shape parameter
    mu += Intercept + Xc * b;
    xi = scale_xi(tmp_xi, Y, mu, sigma);
    for (n in 1:N) {
      target += gen_extreme_value_lpdf(Y[n] | mu[n], sigma, xi);
    }
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_Intercept = Intercept - dot_product(means_X, b);
}

