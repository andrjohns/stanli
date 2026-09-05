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
  // data for group-level effects of ID 1
  int<lower=1> N_1;  // number of grouping levels
  int<lower=1> M_1;  // number of coefficients per level
  array[N] int<lower=1> J_1;  // grouping indicator per observation
  // group-level predictor values
  vector[N] Z_1_mu2_1;
  // data for group-level effects of ID 2
  int<lower=1> N_2;  // number of grouping levels
  int<lower=1> M_2;  // number of coefficients per level
  array[N] int<lower=1> J_2;  // grouping indicator per observation
  // group-level predictor values
  vector[N] Z_2_mu3_1;
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
  vector<lower=0>[M_1] sd_1;  // group-level standard deviations
  array[M_1] vector[N_1] z_1;  // standardized group-level effects
  vector<lower=0>[M_2] sd_2;  // group-level standard deviations
  array[M_2] vector[N_2] z_2;  // standardized group-level effects
}
transformed parameters {
  vector[N_1] r_1_mu2_1;  // actual group-level effects
  vector[N_2] r_2_mu3_1;  // actual group-level effects
  // prior contributions to the log posterior
  real lprior = 0;
  r_1_mu2_1 = (sd_1[1] * (z_1[1]));
  r_2_mu3_1 = (sd_2[1] * (z_2[1]));
  lprior += student_t_lpdf(Intercept_mu2 | 3, 0, 2.5);
  lprior += student_t_lpdf(Intercept_mu3 | 3, 0, 2.5);
  lprior += student_t_lpdf(sd_1 | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += student_t_lpdf(sd_2 | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] mu2 = rep_vector(0.0, N);
    // initialize linear predictor term
    vector[N] mu3 = rep_vector(0.0, N);
    // linear predictor matrix
    array[N] vector[ncat] mu;
    mu2 += Intercept_mu2 + Xc_mu2 * b_mu2;
    mu3 += Intercept_mu3 + Xc_mu3 * b_mu3;
    for (n in 1:N) {
      // add more terms to the linear predictor
      mu2[n] += r_1_mu2_1[J_1[n]] * Z_1_mu2_1[n];
    }
    for (n in 1:N) {
      // add more terms to the linear predictor
      mu3[n] += r_2_mu3_1[J_2[n]] * Z_2_mu3_1[n];
    }
    for (n in 1:N) {
      mu[n] = transpose([0, mu2[n], mu3[n]]);
    }
    for (n in 1:N) {
      target += categorical_logit_lpmf(Y[n] | mu[n]);
    }
  }
  // priors including constants
  target += lprior;
  target += std_normal_lpdf(z_1[1]);
  target += std_normal_lpdf(z_2[1]);
}
generated quantities {
  // actual population-level intercept
  real b_mu2_Intercept = Intercept_mu2 - dot_product(means_X_mu2, b_mu2);
  // actual population-level intercept
  real b_mu3_Intercept = Intercept_mu3 - dot_product(means_X_mu3, b_mu3);
}

