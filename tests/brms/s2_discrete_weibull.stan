// generated with brms 2.23.0
functions {
 /* discrete Weibull log-PMF for a single response
  * Args:
  *   y: the response value
  *   mu: location parameter on the unit interval
  *   shape: positive shape parameter
  * Returns:
  *   a scalar to be added to the log posterior
  */
  real discrete_weibull_lpmf(int y, real mu, real shape) {
    return log(mu^y^shape - mu^(y+1)^shape);
  }
  // discrete Weibull log-CDF for a single response
  real discrete_weibull_lcdf(int y, real mu, real shape) {
    return log1m(mu^(y + 1)^shape);
  }
  // discrete Weibull log-CCDF for a single response
  real discrete_weibull_lccdf(int y, real mu, real shape) {
    return lmultiply((y + 1)^shape, mu);
  }
}
data {
  int<lower=1> N;  // total number of observations
  array[N] int Y;  // response variable
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
  lprior += student_t_lpdf(Intercept | 3, 0, 2.5);
  lprior += gamma_lpdf(shape | 0.01, 0.01);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] mu = rep_vector(0.0, N);
    mu += Intercept + Xc * b;
    mu = inv_logit(mu);
    for (n in 1:N) {
      target += discrete_weibull_lpmf(Y[n] | mu[n], shape);
    }
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_Intercept = Intercept - dot_product(means_X, b);
}

