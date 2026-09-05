// generated with brms 2.23.0
functions {
}
data {
  int<lower=1> N;  // total number of observations
  int<lower=1> N_y;  // number of observations
  vector[N_y] Y_y;  // response variable
  int<lower=1> K_y;  // number of population-level effects
  matrix[N_y, K_y] X_y;  // population-level design matrix
  int<lower=1> Kc_y;  // number of population-level effects after centering
  int<lower=1> N_ypos;  // number of observations
  vector[N_ypos] Y_ypos;  // response variable
  int<lower=1> K_ypos;  // number of population-level effects
  matrix[N_ypos, K_ypos] X_ypos;  // population-level design matrix
  int<lower=1> Kc_ypos;  // number of population-level effects after centering
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
  matrix[N_y, Kc_y] Xc_y;  // centered version of X_y without an intercept
  vector[Kc_y] means_X_y;  // column means of X_y before centering
  matrix[N_ypos, Kc_ypos] Xc_ypos;  // centered version of X_ypos without an intercept
  vector[Kc_ypos] means_X_ypos;  // column means of X_ypos before centering
  for (i in 2:K_y) {
    means_X_y[i - 1] = mean(X_y[, i]);
    Xc_y[, i - 1] = X_y[, i] - means_X_y[i - 1];
  }
  for (i in 2:K_ypos) {
    means_X_ypos[i - 1] = mean(X_ypos[, i]);
    Xc_ypos[, i - 1] = X_ypos[, i] - means_X_ypos[i - 1];
  }
}
parameters {
  vector[Kc_y] b_y;  // regression coefficients
  real Intercept_y;  // temporary intercept for centered predictors
  real<lower=0> sigma_y;  // dispersion parameter
  vector[Kc_ypos] b_ypos;  // regression coefficients
  real Intercept_ypos;  // temporary intercept for centered predictors
  real<lower=0> sigma_ypos;  // dispersion parameter
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept_y | 3, 0.2, 2.5);
  lprior += student_t_lpdf(sigma_y | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += student_t_lpdf(Intercept_ypos | 3, 0.8, 2.5);
  lprior += student_t_lpdf(sigma_ypos | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
}
model {
  // likelihood including constants
  if (!prior_only) {
    target += normal_id_glm_lpdf(Y_y | Xc_y, Intercept_y, b_y, sigma_y);
    target += normal_id_glm_lpdf(Y_ypos | Xc_ypos, Intercept_ypos, b_ypos, sigma_ypos);
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_y_Intercept = Intercept_y - dot_product(means_X_y, b_y);
  // actual population-level intercept
  real b_ypos_Intercept = Intercept_ypos - dot_product(means_X_ypos, b_ypos);
}

