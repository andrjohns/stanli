// generated with brms 2.23.0
functions {
}
data {
  int<lower=1> N;  // total number of observations
  vector[N] Y;  // response variable
  int<lower=1> K_mu1;  // number of population-level effects
  matrix[N, K_mu1] X_mu1;  // population-level design matrix
  int<lower=1> Kc_mu1;  // number of population-level effects after centering
  int<lower=1> K_mu2;  // number of population-level effects
  matrix[N, K_mu2] X_mu2;  // population-level design matrix
  int<lower=1> Kc_mu2;  // number of population-level effects after centering
  int<lower=1> K_theta1;  // number of population-level effects
  matrix[N, K_theta1] X_theta1;  // population-level design matrix
  int<lower=1> Kc_theta1;  // number of population-level effects after centering
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
  matrix[N, Kc_mu1] Xc_mu1;  // centered version of X_mu1 without an intercept
  vector[Kc_mu1] means_X_mu1;  // column means of X_mu1 before centering
  matrix[N, Kc_mu2] Xc_mu2;  // centered version of X_mu2 without an intercept
  vector[Kc_mu2] means_X_mu2;  // column means of X_mu2 before centering
  matrix[N, Kc_theta1] Xc_theta1;  // centered version of X_theta1 without an intercept
  vector[Kc_theta1] means_X_theta1;  // column means of X_theta1 before centering
  for (i in 2:K_mu1) {
    means_X_mu1[i - 1] = mean(X_mu1[, i]);
    Xc_mu1[, i - 1] = X_mu1[, i] - means_X_mu1[i - 1];
  }
  for (i in 2:K_mu2) {
    means_X_mu2[i - 1] = mean(X_mu2[, i]);
    Xc_mu2[, i - 1] = X_mu2[, i] - means_X_mu2[i - 1];
  }
  for (i in 2:K_theta1) {
    means_X_theta1[i - 1] = mean(X_theta1[, i]);
    Xc_theta1[, i - 1] = X_theta1[, i] - means_X_theta1[i - 1];
  }
}
parameters {
  vector[Kc_mu1] b_mu1;  // regression coefficients
  real<lower=0> sigma1;  // dispersion parameter
  vector[Kc_mu2] b_mu2;  // regression coefficients
  real<lower=0> sigma2;  // dispersion parameter
  vector[Kc_theta1] b_theta1;  // regression coefficients
  real Intercept_theta1;  // temporary intercept for centered predictors
  ordered[2] ordered_Intercept;  // to identify mixtures
}
transformed parameters {
  // identify mixtures via ordering of the intercepts
  real Intercept_mu1 = ordered_Intercept[1];
  // identify mixtures via ordering of the intercepts
  real Intercept_mu2 = ordered_Intercept[2];
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept_mu1 | 3, -0.3, 2.5);
  lprior += student_t_lpdf(sigma1 | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += student_t_lpdf(Intercept_mu2 | 3, -0.3, 2.5);
  lprior += student_t_lpdf(sigma2 | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += logistic_lpdf(Intercept_theta1 | 0, 1);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] mu1 = rep_vector(0.0, N);
    // initialize linear predictor term
    vector[N] mu2 = rep_vector(0.0, N);
    // initialize linear predictor term
    vector[N] theta1 = rep_vector(0.0, N);
    vector[N] theta2 = rep_vector(0.0, N);
    real log_sum_exp_theta;
    mu1 += Intercept_mu1 + Xc_mu1 * b_mu1;
    mu2 += Intercept_mu2 + Xc_mu2 * b_mu2;
    theta1 += Intercept_theta1 + Xc_theta1 * b_theta1;
    for (n in 1:N) {
      // scale theta to become a probability vector
      log_sum_exp_theta = log(exp(theta1[n]) + exp(theta2[n]));
      theta1[n] = theta1[n] - log_sum_exp_theta;
      theta2[n] = theta2[n] - log_sum_exp_theta;
    }
    // likelihood of the mixture model
    for (n in 1:N) {
      array[2] real ps;
      ps[1] = theta1[n] + normal_lpdf(Y[n] | mu1[n], sigma1);
      ps[2] = theta2[n] + normal_lpdf(Y[n] | mu2[n], sigma2);
      target += log_sum_exp(ps);
    }
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_mu1_Intercept = Intercept_mu1 - dot_product(means_X_mu1, b_mu1);
  // actual population-level intercept
  real b_mu2_Intercept = Intercept_mu2 - dot_product(means_X_mu2, b_mu2);
  // actual population-level intercept
  real b_theta1_Intercept = Intercept_theta1 - dot_product(means_X_theta1, b_theta1);
}

