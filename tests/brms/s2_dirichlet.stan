// generated with brms 2.23.0
functions {
  /* dirichlet-logit log-PDF
   * Args:
   *   y: vector of real response values
   *   mu: vector of category logit probabilities
   *   phi: precision parameter
   * Returns:
   *   a scalar to be added to the log posterior
   */
   real dirichlet_logit_lpdf(vector y, vector mu, real phi) {
     return dirichlet_lpdf(y | softmax(mu) * phi);
   }
}
data {
  int<lower=1> N;  // total number of observations
  int<lower=2> ncat;  // number of categories
  array[N] vector[ncat] Y;  // response array
  int<lower=1> K_mus2;  // number of population-level effects
  matrix[N, K_mus2] X_mus2;  // population-level design matrix
  int<lower=1> Kc_mus2;  // number of population-level effects after centering
  int<lower=1> K_mus3;  // number of population-level effects
  matrix[N, K_mus3] X_mus3;  // population-level design matrix
  int<lower=1> Kc_mus3;  // number of population-level effects after centering
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
  matrix[N, Kc_mus2] Xc_mus2;  // centered version of X_mus2 without an intercept
  vector[Kc_mus2] means_X_mus2;  // column means of X_mus2 before centering
  matrix[N, Kc_mus3] Xc_mus3;  // centered version of X_mus3 without an intercept
  vector[Kc_mus3] means_X_mus3;  // column means of X_mus3 before centering
  for (i in 2:K_mus2) {
    means_X_mus2[i - 1] = mean(X_mus2[, i]);
    Xc_mus2[, i - 1] = X_mus2[, i] - means_X_mus2[i - 1];
  }
  for (i in 2:K_mus3) {
    means_X_mus3[i - 1] = mean(X_mus3[, i]);
    Xc_mus3[, i - 1] = X_mus3[, i] - means_X_mus3[i - 1];
  }
}
parameters {
  vector[Kc_mus2] b_mus2;  // regression coefficients
  real Intercept_mus2;  // temporary intercept for centered predictors
  vector[Kc_mus3] b_mus3;  // regression coefficients
  real Intercept_mus3;  // temporary intercept for centered predictors
  real<lower=0> phi;  // precision parameter
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept_mus2 | 3, 0, 2.5);
  lprior += student_t_lpdf(Intercept_mus3 | 3, 0, 2.5);
  lprior += gamma_lpdf(phi | 0.01, 0.01);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] mus2 = rep_vector(0.0, N);
    // initialize linear predictor term
    vector[N] mus3 = rep_vector(0.0, N);
    // linear predictor matrix
    array[N] vector[ncat] mu;
    mus2 += Intercept_mus2 + Xc_mus2 * b_mus2;
    mus3 += Intercept_mus3 + Xc_mus3 * b_mus3;
    for (n in 1:N) {
      mu[n] = transpose([0, mus2[n], mus3[n]]);
    }
    for (n in 1:N) {
      target += dirichlet_logit_lpdf(Y[n] | mu[n], phi);
    }
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_mus2_Intercept = Intercept_mus2 - dot_product(means_X_mus2, b_mus2);
  // actual population-level intercept
  real b_mus3_Intercept = Intercept_mus3 - dot_product(means_X_mus3, b_mus3);
}

