// generated with brms 2.23.0
functions {
}
data {
  int<lower=1> N;  // total number of observations
  int<lower=1> N_ymi;  // number of observations
  vector[N_ymi] Y_ymi;  // response variable
  int<lower=0> Nmi_ymi;  // number of missings
  array[Nmi_ymi] int<lower=1> Jmi_ymi;  // positions of missings
  int<lower=1> K_ymi;  // number of population-level effects
  matrix[N_ymi, K_ymi] X_ymi;  // population-level design matrix
  int<lower=1> Kc_ymi;  // number of population-level effects after centering
  int<lower=1> N_x;  // number of observations
  vector[N_x] Y_x;  // response variable
  int<lower=1> K_x;  // number of population-level effects
  matrix[N_x, K_x] X_x;  // population-level design matrix
  int<lower=1> Kc_x;  // number of population-level effects after centering
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
  matrix[N_ymi, Kc_ymi] Xc_ymi;  // centered version of X_ymi without an intercept
  vector[Kc_ymi] means_X_ymi;  // column means of X_ymi before centering
  matrix[N_x, Kc_x] Xc_x;  // centered version of X_x without an intercept
  vector[Kc_x] means_X_x;  // column means of X_x before centering
  for (i in 2:K_ymi) {
    means_X_ymi[i - 1] = mean(X_ymi[, i]);
    Xc_ymi[, i - 1] = X_ymi[, i] - means_X_ymi[i - 1];
  }
  for (i in 2:K_x) {
    means_X_x[i - 1] = mean(X_x[, i]);
    Xc_x[, i - 1] = X_x[, i] - means_X_x[i - 1];
  }
}
parameters {
  vector[Nmi_ymi] Ymi_ymi;  // estimated missings
  vector[Kc_ymi] b_ymi;  // regression coefficients
  real Intercept_ymi;  // temporary intercept for centered predictors
  real<lower=0> sigma_ymi;  // dispersion parameter
  vector[Kc_x] b_x;  // regression coefficients
  real Intercept_x;  // temporary intercept for centered predictors
  real<lower=0> sigma_x;  // dispersion parameter
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept_ymi | 3, 0, 2.5);
  lprior += student_t_lpdf(sigma_ymi | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += student_t_lpdf(Intercept_x | 3, 0.2, 2.5);
  lprior += student_t_lpdf(sigma_x | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // vector combining observed and missing responses
    vector[N_ymi] Yl_ymi = Y_ymi;
    Yl_ymi[Jmi_ymi] = Ymi_ymi;
    target += normal_id_glm_lpdf(Yl_ymi | Xc_ymi, Intercept_ymi, b_ymi, sigma_ymi);
    target += normal_id_glm_lpdf(Y_x | Xc_x, Intercept_x, b_x, sigma_x);
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_ymi_Intercept = Intercept_ymi - dot_product(means_X_ymi, b_ymi);
  // actual population-level intercept
  real b_x_Intercept = Intercept_x - dot_product(means_X_x, b_x);
}

