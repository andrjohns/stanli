// generated with brms 2.23.0
functions {
}
data {
  int<lower=1> N;  // total number of observations
  int<lower=1> N_bmi;  // number of observations
  vector[N_bmi] Y_bmi;  // response variable
  int<lower=0> Nmi_bmi;  // number of missings
  array[Nmi_bmi] int<lower=1> Jmi_bmi;  // positions of missings
  int<lower=1> K_bmi;  // number of population-level effects
  matrix[N_bmi, K_bmi] X_bmi;  // population-level design matrix
  int<lower=1> Kc_bmi;  // number of population-level effects after centering
  int<lower=1> Ksp_bmi;  // number of special effects terms
  // covariates of special effects terms
  vector[N_bmi] Csp_bmi_1;
  int<lower=1> N_chl;  // number of observations
  vector[N_chl] Y_chl;  // response variable
  int<lower=0> Nmi_chl;  // number of missings
  array[Nmi_chl] int<lower=1> Jmi_chl;  // positions of missings
  int<lower=1> K_chl;  // number of population-level effects
  matrix[N_chl, K_chl] X_chl;  // population-level design matrix
  int<lower=1> Kc_chl;  // number of population-level effects after centering
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
  matrix[N_bmi, Kc_bmi] Xc_bmi;  // centered version of X_bmi without an intercept
  vector[Kc_bmi] means_X_bmi;  // column means of X_bmi before centering
  matrix[N_chl, Kc_chl] Xc_chl;  // centered version of X_chl without an intercept
  vector[Kc_chl] means_X_chl;  // column means of X_chl before centering
  for (i in 2:K_bmi) {
    means_X_bmi[i - 1] = mean(X_bmi[, i]);
    Xc_bmi[, i - 1] = X_bmi[, i] - means_X_bmi[i - 1];
  }
  for (i in 2:K_chl) {
    means_X_chl[i - 1] = mean(X_chl[, i]);
    Xc_chl[, i - 1] = X_chl[, i] - means_X_chl[i - 1];
  }
}
parameters {
  vector[Nmi_bmi] Ymi_bmi;  // estimated missings
  vector[Kc_bmi] b_bmi;  // regression coefficients
  real Intercept_bmi;  // temporary intercept for centered predictors
  vector[Ksp_bmi] bsp_bmi;  // special effects coefficients
  real<lower=0> sigma_bmi;  // dispersion parameter
  vector[Nmi_chl] Ymi_chl;  // estimated missings
  vector[Kc_chl] b_chl;  // regression coefficients
  real Intercept_chl;  // temporary intercept for centered predictors
  real<lower=0> sigma_chl;  // dispersion parameter
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept_bmi | 3, 0, 2.5);
  lprior += student_t_lpdf(sigma_bmi | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += student_t_lpdf(Intercept_chl | 3, 0, 2.5);
  lprior += student_t_lpdf(sigma_chl | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // vector combining observed and missing responses
    vector[N_bmi] Yl_bmi = Y_bmi;
    // vector combining observed and missing responses
    vector[N_chl] Yl_chl = Y_chl;
    // initialize linear predictor term
    vector[N_bmi] mu_bmi = rep_vector(0.0, N_bmi);
    Yl_bmi[Jmi_bmi] = Ymi_bmi;
    Yl_chl[Jmi_chl] = Ymi_chl;
    mu_bmi += Intercept_bmi;
    for (n in 1:N_bmi) {
      // add more terms to the linear predictor
      mu_bmi[n] += (bsp_bmi[1]) * Yl_chl[n] + (bsp_bmi[2]) * Yl_chl[n] * Csp_bmi_1[n];
    }
    target += normal_id_glm_lpdf(Yl_bmi | Xc_bmi, mu_bmi, b_bmi, sigma_bmi);
    target += normal_id_glm_lpdf(Yl_chl | Xc_chl, Intercept_chl, b_chl, sigma_chl);
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_bmi_Intercept = Intercept_bmi - dot_product(means_X_bmi, b_bmi);
  // actual population-level intercept
  real b_chl_Intercept = Intercept_chl - dot_product(means_X_chl, b_chl);
}

