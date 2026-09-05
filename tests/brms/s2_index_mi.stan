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
  int<lower=1> N_y;  // number of observations
  vector[N_y] Y_y;  // response variable
  int<lower=1> Ksp_y;  // number of special effects terms
  array[N_y] int idxl_y_yposmi_1;  // matching indices
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
  matrix[N_yposmi, Kc_yposmi] Xc_yposmi;  // centered version of X_yposmi without an intercept
  vector[Kc_yposmi] means_X_yposmi;  // column means of X_yposmi before centering
  for (i in 2:K_yposmi) {
    means_X_yposmi[i - 1] = mean(X_yposmi[, i]);
    Xc_yposmi[, i - 1] = X_yposmi[, i] - means_X_yposmi[i - 1];
  }
}
parameters {
  vector[Nmi_yposmi] Ymi_yposmi;  // estimated missings
  vector[Kc_yposmi] b_yposmi;  // regression coefficients
  real Intercept_yposmi;  // temporary intercept for centered predictors
  real<lower=0> sigma_yposmi;  // dispersion parameter
  real Intercept_y;  // temporary intercept for centered predictors
  vector[Ksp_y] bsp_y;  // special effects coefficients
  real<lower=0> sigma_y;  // dispersion parameter
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept_yposmi | 3, 0, 2.5);
  lprior += student_t_lpdf(sigma_yposmi | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += student_t_lpdf(Intercept_y | 3, -0.3, 2.5);
  lprior += student_t_lpdf(sigma_y | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // vector combining observed and missing responses
    vector[N_yposmi] Yl_yposmi = Y_yposmi;
    // initialize linear predictor term
    vector[N_y] mu_y = rep_vector(0.0, N_y);
    Yl_yposmi[Jmi_yposmi] = Ymi_yposmi;
    mu_y += Intercept_y;
    for (n in 1:N_y) {
      // add more terms to the linear predictor
      mu_y[n] += (bsp_y[1]) * Yl_yposmi[idxl_y_yposmi_1[n]];
    }
    target += normal_id_glm_lpdf(Yl_yposmi | Xc_yposmi, Intercept_yposmi, b_yposmi, sigma_yposmi);
    target += normal_lpdf(Y_y | mu_y, sigma_y);
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_yposmi_Intercept = Intercept_yposmi - dot_product(means_X_yposmi, b_yposmi);
  // actual population-level intercept
  real b_y_Intercept = Intercept_y;
}

