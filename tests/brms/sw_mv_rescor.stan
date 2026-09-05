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
  int<lower=1> nresp;  // number of responses
  int nrescor;  // number of residual correlations
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
  matrix[N_y, Kc_y] Xc_y;  // centered version of X_y without an intercept
  vector[Kc_y] means_X_y;  // column means of X_y before centering
  matrix[N_ypos, Kc_ypos] Xc_ypos;  // centered version of X_ypos without an intercept
  vector[Kc_ypos] means_X_ypos;  // column means of X_ypos before centering
  array[N] vector[nresp] Y;  // response array
  for (i in 2:K_y) {
    means_X_y[i - 1] = mean(X_y[, i]);
    Xc_y[, i - 1] = X_y[, i] - means_X_y[i - 1];
  }
  for (i in 2:K_ypos) {
    means_X_ypos[i - 1] = mean(X_ypos[, i]);
    Xc_ypos[, i - 1] = X_ypos[, i] - means_X_ypos[i - 1];
  }
  for (n in 1:N) {
    Y[n] = transpose([Y_y[n], Y_ypos[n]]);
  }
}
parameters {
  vector[Kc_y] b_y;  // regression coefficients
  real Intercept_y;  // temporary intercept for centered predictors
  real<lower=0> sigma_y;  // dispersion parameter
  vector[Kc_ypos] b_ypos;  // regression coefficients
  real Intercept_ypos;  // temporary intercept for centered predictors
  real<lower=0> sigma_ypos;  // dispersion parameter
  cholesky_factor_corr[nresp] Lrescor;  // parameters for multivariate linear models
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
  lprior += lkj_corr_cholesky_lpdf(Lrescor | 1);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N_y] mu_y = rep_vector(0.0, N_y);
    // initialize linear predictor term
    vector[N_ypos] mu_ypos = rep_vector(0.0, N_ypos);
    // multivariate predictor array
    array[N] vector[nresp] Mu;
    vector[nresp] sigma = transpose([sigma_y, sigma_ypos]);
    // cholesky factor of residual covariance matrix
    matrix[nresp, nresp] LSigma = diag_pre_multiply(sigma, Lrescor);
    mu_y += Intercept_y + Xc_y * b_y;
    mu_ypos += Intercept_ypos + Xc_ypos * b_ypos;
    // combine univariate parameters
    for (n in 1:N) {
      Mu[n] = transpose([mu_y[n], mu_ypos[n]]);
    }
    target += multi_normal_cholesky_lpdf(Y | Mu, LSigma);
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_y_Intercept = Intercept_y - dot_product(means_X_y, b_y);
  // actual population-level intercept
  real b_ypos_Intercept = Intercept_ypos - dot_product(means_X_ypos, b_ypos);
  // residual correlations
  corr_matrix[nresp] Rescor = multiply_lower_tri_self_transpose(Lrescor);
  vector<lower=-1,upper=1>[nrescor] rescor;
  // extract upper diagonal of correlation matrix
  for (k in 1:nresp) {
    for (j in 1:(k - 1)) {
      rescor[choose(k - 1, 2) + j] = Rescor[j, k];
    }
  }
}

