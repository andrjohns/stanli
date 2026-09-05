// generated with brms 2.23.0
functions {
}
data {
  int<lower=1> N;  // total number of observations
  int<lower=2> ncat;  // number of categories
  array[N] int Y;  // response variable
  int<lower=1> K_mu2;  // number of population-level effects
  matrix[N, K_mu2] X_mu2;  // population-level design matrix
  int<lower=1> Kc_mu2;  // number of population-level effects after centering
  int<lower=1> K_mu3;  // number of population-level effects
  matrix[N, K_mu3] X_mu3;  // population-level design matrix
  int<lower=1> Kc_mu3;  // number of population-level effects after centering
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
  matrix[N, Kc_mu2] Xc_mu2;  // centered version of X_mu2 without an intercept
  vector[Kc_mu2] means_X_mu2;  // column means of X_mu2 before centering
  matrix[N, Kc_mu3] Xc_mu3;  // centered version of X_mu3 without an intercept
  vector[Kc_mu3] means_X_mu3;  // column means of X_mu3 before centering
  for (i in 2:K_mu2) {
    means_X_mu2[i - 1] = mean(X_mu2[, i]);
    Xc_mu2[, i - 1] = X_mu2[, i] - means_X_mu2[i - 1];
  }
  for (i in 2:K_mu3) {
    means_X_mu3[i - 1] = mean(X_mu3[, i]);
    Xc_mu3[, i - 1] = X_mu3[, i] - means_X_mu3[i - 1];
  }
}
parameters {
  vector[Kc_mu2] b_mu2;  // regression coefficients
  real Intercept_mu2;  // temporary intercept for centered predictors
  vector[Kc_mu3] b_mu3;  // regression coefficients
  real Intercept_mu3;  // temporary intercept for centered predictors
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept_mu2 | 3, 0, 2.5);
  lprior += student_t_lpdf(Intercept_mu3 | 3, 0, 2.5);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // joint regression coefficients over categories
    matrix[Kc_mu2, ncat] b;
    // joint intercepts over categories
    vector[ncat] Intercept;
    b[, 1] = rep_vector(0, Kc_mu2);
    b[, 2] = b_mu2;
    b[, 3] = b_mu3;
    Intercept = transpose([0, Intercept_mu2, Intercept_mu3]);
    target += categorical_logit_glm_lpmf(Y | Xc_mu2, Intercept, b);
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_mu2_Intercept = Intercept_mu2 - dot_product(means_X_mu2, b_mu2);
  // actual population-level intercept
  real b_mu3_Intercept = Intercept_mu3 - dot_product(means_X_mu3, b_mu3);
}

