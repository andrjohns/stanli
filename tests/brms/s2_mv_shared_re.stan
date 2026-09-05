// generated with brms 2.23.0
functions {
 /* compute correlated group-level effects
  * Args:
  *   z: matrix of unscaled group-level effects
  *   SD: vector of standard deviation parameters
  *   L: cholesky factor correlation matrix
  * Returns:
  *   matrix of scaled group-level effects
  */
  matrix scale_r_cor(matrix z, vector SD, matrix L) {
    // r is stored in another dimension order than z
    return transpose(diag_pre_multiply(SD, L) * z);
  }
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
  // data for group-level effects of ID 1
  int<lower=1> N_1;  // number of grouping levels
  int<lower=1> M_1;  // number of coefficients per level
  array[N_y] int<lower=1> J_1_y;  // grouping indicator per observation
  array[N_ypos] int<lower=1> J_1_ypos;  // grouping indicator per observation
  // group-level predictor values
  vector[N_y] Z_1_y_1;
  vector[N_ypos] Z_1_ypos_2;
  int<lower=1> NC_1;  // number of group-level correlations
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
  vector<lower=0>[M_1] sd_1;  // group-level standard deviations
  matrix[M_1, N_1] z_1;  // standardized group-level effects
  cholesky_factor_corr[M_1] L_1;  // cholesky factor of correlation matrix
}
transformed parameters {
  matrix[N_1, M_1] r_1;  // actual group-level effects
  // using vectors speeds up indexing in loops
  vector[N_1] r_1_y_1;
  vector[N_1] r_1_ypos_2;
  // prior contributions to the log posterior
  real lprior = 0;
  // compute actual group-level effects
  r_1 = scale_r_cor(z_1, sd_1, L_1);
  r_1_y_1 = r_1[, 1];
  r_1_ypos_2 = r_1[, 2];
  lprior += student_t_lpdf(Intercept_y | 3, -0.3, 2.5);
  lprior += student_t_lpdf(sigma_y | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += student_t_lpdf(Intercept_ypos | 3, 0.6, 2.5);
  lprior += student_t_lpdf(sigma_ypos | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += student_t_lpdf(sd_1 | 3, 0, 2.5)
    - 2 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += lkj_corr_cholesky_lpdf(L_1 | 1);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N_y] mu_y = rep_vector(0.0, N_y);
    // initialize linear predictor term
    vector[N_ypos] mu_ypos = rep_vector(0.0, N_ypos);
    mu_y += Intercept_y;
    mu_ypos += Intercept_ypos;
    for (n in 1:N_y) {
      // add more terms to the linear predictor
      mu_y[n] += r_1_y_1[J_1_y[n]] * Z_1_y_1[n];
    }
    for (n in 1:N_ypos) {
      // add more terms to the linear predictor
      mu_ypos[n] += r_1_ypos_2[J_1_ypos[n]] * Z_1_ypos_2[n];
    }
    target += normal_id_glm_lpdf(Y_y | Xc_y, mu_y, b_y, sigma_y);
    target += normal_id_glm_lpdf(Y_ypos | Xc_ypos, mu_ypos, b_ypos, sigma_ypos);
  }
  // priors including constants
  target += lprior;
  target += std_normal_lpdf(to_vector(z_1));
}
generated quantities {
  // actual population-level intercept
  real b_y_Intercept = Intercept_y - dot_product(means_X_y, b_y);
  // actual population-level intercept
  real b_ypos_Intercept = Intercept_ypos - dot_product(means_X_ypos, b_ypos);
  // compute group-level correlations
  corr_matrix[M_1] Cor_1 = multiply_lower_tri_self_transpose(L_1);
  vector<lower=-1,upper=1>[NC_1] cor_1;
  // extract upper diagonal of correlation matrix
  for (k in 1:M_1) {
    for (j in 1:(k - 1)) {
      cor_1[choose(k - 1, 2) + j] = Cor_1[j, k];
    }
  }
}

