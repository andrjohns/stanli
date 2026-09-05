// generated with brms 2.23.0
functions {
  /* zero-one-inflated beta log-PDF of a single response
   * Args:
   *   y: response value
   *   mu: mean parameter of the beta part
   *   phi: precision parameter of the beta part
   *   zoi: zero-one-inflation probability
   *   coi: conditional one-inflation probability
   * Returns:
   *   a scalar to be added to the log posterior
   */
   real zero_one_inflated_beta_lpdf(real y, real mu, real phi,
                                    real zoi, real coi) {
     row_vector[2] shape = [mu * phi, (1 - mu) * phi];
     if (y == 0) {
       return bernoulli_lpmf(1 | zoi) + bernoulli_lpmf(0 | coi);
     } else if (y == 1) {
       return bernoulli_lpmf(1 | zoi) + bernoulli_lpmf(1 | coi);
     } else {
       return bernoulli_lpmf(0 | zoi) + beta_lpdf(y | shape[1], shape[2]);
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
  real<lower=0> phi;  // precision parameter
  real<lower=0,upper=1> zoi;  // zero-one-inflation probability
  real<lower=0,upper=1> coi;  // conditional one-inflation probability
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept | 3, 0, 2.5);
  lprior += gamma_lpdf(phi | 0.01, 0.01);
  lprior += beta_lpdf(zoi | 1, 1);
  lprior += beta_lpdf(coi | 1, 1);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] mu = rep_vector(0.0, N);
    mu += Intercept + Xc * b;
    mu = inv_logit(mu);
    for (n in 1:N) {
      target += zero_one_inflated_beta_lpdf(Y[n] | mu[n], phi, zoi, coi);
    }
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_Intercept = Intercept - dot_product(means_X, b);
}

