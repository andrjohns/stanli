// generated with brms 2.23.0
functions {
  /* multinomial-logit log-PMF
   * Args:
   *   y: array of integer response values
   *   mu: vector of category logit probabilities
   * Returns:
   *   a scalar to be added to the log posterior
   */
   real multinomial_logit2_lpmf(array[] int y, vector mu) {
     return multinomial_lpmf(y | softmax(mu));
   }
}
data {
  int<lower=1> N;  // total number of observations
  int<lower=2> ncat;  // number of categories
  array[N, ncat] int Y;  // response array
  array[N] int trials;  // number of trials
  int<lower=1> K_muc2;  // number of population-level effects
  matrix[N, K_muc2] X_muc2;  // population-level design matrix
  int<lower=1> Kc_muc2;  // number of population-level effects after centering
  int<lower=1> K_muc3;  // number of population-level effects
  matrix[N, K_muc3] X_muc3;  // population-level design matrix
  int<lower=1> Kc_muc3;  // number of population-level effects after centering
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
  matrix[N, Kc_muc2] Xc_muc2;  // centered version of X_muc2 without an intercept
  vector[Kc_muc2] means_X_muc2;  // column means of X_muc2 before centering
  matrix[N, Kc_muc3] Xc_muc3;  // centered version of X_muc3 without an intercept
  vector[Kc_muc3] means_X_muc3;  // column means of X_muc3 before centering
  for (i in 2:K_muc2) {
    means_X_muc2[i - 1] = mean(X_muc2[, i]);
    Xc_muc2[, i - 1] = X_muc2[, i] - means_X_muc2[i - 1];
  }
  for (i in 2:K_muc3) {
    means_X_muc3[i - 1] = mean(X_muc3[, i]);
    Xc_muc3[, i - 1] = X_muc3[, i] - means_X_muc3[i - 1];
  }
}
parameters {
  vector[Kc_muc2] b_muc2;  // regression coefficients
  real Intercept_muc2;  // temporary intercept for centered predictors
  vector[Kc_muc3] b_muc3;  // regression coefficients
  real Intercept_muc3;  // temporary intercept for centered predictors
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept_muc2 | 3, 0, 2.5);
  lprior += student_t_lpdf(Intercept_muc3 | 3, 0, 2.5);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] muc2 = rep_vector(0.0, N);
    // initialize linear predictor term
    vector[N] muc3 = rep_vector(0.0, N);
    // linear predictor matrix
    array[N] vector[ncat] mu;
    muc2 += Intercept_muc2 + Xc_muc2 * b_muc2;
    muc3 += Intercept_muc3 + Xc_muc3 * b_muc3;
    for (n in 1:N) {
      mu[n] = transpose([0, muc2[n], muc3[n]]);
    }
    for (n in 1:N) {
      target += multinomial_logit2_lpmf(Y[n] | mu[n]);
    }
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_muc2_Intercept = Intercept_muc2 - dot_product(means_X_muc2, b_muc2);
  // actual population-level intercept
  real b_muc3_Intercept = Intercept_muc3 - dot_product(means_X_muc3, b_muc3);
}

