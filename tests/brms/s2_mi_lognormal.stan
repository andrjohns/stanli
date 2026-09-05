// generated with brms 2.23.0
functions {
}
data {
  int<lower=1> N;  // total number of observations
  int<lower=1> N_yposmi;  // number of observations
  vector[N_yposmi] Y_yposmi;  // response variable
  int<lower=0> Nmi_yposmi;  // number of missings
  array[Nmi_yposmi] int<lower=1> Jmi_yposmi;  // positions of missings
  int<lower=1> K_yposmi;  // number of population-level effects
  matrix[N_yposmi, K_yposmi] X_yposmi;  // population-level design matrix
  int<lower=1> Kc_yposmi;  // number of population-level effects after centering
  int<lower=1> N_x;  // number of observations
  vector[N_x] Y_x;  // response variable
  int<lower=1> K_x;  // number of population-level effects
  matrix[N_x, K_x] X_x;  // population-level design matrix
  int<lower=1> Kc_x;  // number of population-level effects after centering
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
  matrix[N_yposmi, Kc_yposmi] Xc_yposmi;  // centered version of X_yposmi without an intercept
  vector[Kc_yposmi] means_X_yposmi;  // column means of X_yposmi before centering
  matrix[N_x, Kc_x] Xc_x;  // centered version of X_x without an intercept
  vector[Kc_x] means_X_x;  // column means of X_x before centering
  for (i in 2:K_yposmi) {
    means_X_yposmi[i - 1] = mean(X_yposmi[, i]);
    Xc_yposmi[, i - 1] = X_yposmi[, i] - means_X_yposmi[i - 1];
  }
  for (i in 2:K_x) {
    means_X_x[i - 1] = mean(X_x[, i]);
    Xc_x[, i - 1] = X_x[, i] - means_X_x[i - 1];
  }
}
parameters {
  vector<lower=0>[Nmi_yposmi] Ymi_yposmi;  // estimated missings
  vector[Kc_yposmi] b_yposmi;  // regression coefficients
  real Intercept_yposmi;  // temporary intercept for centered predictors
  real<lower=0> sigma_yposmi;  // dispersion parameter
  vector[Kc_x] b_x;  // regression coefficients
  real Intercept_x;  // temporary intercept for centered predictors
  real<lower=0> sigma_x;  // dispersion parameter
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept_yposmi | 3, 0, 2.5);
  lprior += student_t_lpdf(sigma_yposmi | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += student_t_lpdf(Intercept_x | 3, 0, 2.5);
  lprior += student_t_lpdf(sigma_x | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // vector combining observed and missing responses
    vector[N_yposmi] Yl_yposmi = Y_yposmi;
    // initialize linear predictor term
    vector[N_yposmi] mu_yposmi = rep_vector(0.0, N_yposmi);
    Yl_yposmi[Jmi_yposmi] = Ymi_yposmi;
    mu_yposmi += Intercept_yposmi + Xc_yposmi * b_yposmi;
    target += lognormal_lpdf(Yl_yposmi | mu_yposmi, sigma_yposmi);
    target += normal_id_glm_lpdf(Y_x | Xc_x, Intercept_x, b_x, sigma_x);
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_yposmi_Intercept = Intercept_yposmi - dot_product(means_X_yposmi, b_yposmi);
  // actual population-level intercept
  real b_x_Intercept = Intercept_x - dot_product(means_X_x, b_x);
}

