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
  vector[2] con_theta;  // prior concentration
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
  matrix[N, Kc_mu1] Xc_mu1;  // centered version of X_mu1 without an intercept
  vector[Kc_mu1] means_X_mu1;  // column means of X_mu1 before centering
  matrix[N, Kc_mu2] Xc_mu2;  // centered version of X_mu2 without an intercept
  vector[Kc_mu2] means_X_mu2;  // column means of X_mu2 before centering
  for (i in 2:K_mu1) {
    means_X_mu1[i - 1] = mean(X_mu1[, i]);
    Xc_mu1[, i - 1] = X_mu1[, i] - means_X_mu1[i - 1];
  }
  for (i in 2:K_mu2) {
    means_X_mu2[i - 1] = mean(X_mu2[, i]);
    Xc_mu2[, i - 1] = X_mu2[, i] - means_X_mu2[i - 1];
  }
}
parameters {
  vector[Kc_mu1] b_mu1;  // regression coefficients
  real<lower=0> sigma1;  // dispersion parameter
  vector[Kc_mu2] b_mu2;  // regression coefficients
  real<lower=0> sigma2;  // dispersion parameter
  simplex[2] theta;  // mixing proportions
  ordered[2] ordered_Intercept;  // to identify mixtures
}
transformed parameters {
  // identify mixtures via ordering of the intercepts
  real Intercept_mu1 = ordered_Intercept[1];
  // identify mixtures via ordering of the intercepts
  real Intercept_mu2 = ordered_Intercept[2];
  // mixing proportions
  real<lower=0,upper=1> theta1;
  real<lower=0,upper=1> theta2;
  // prior contributions to the log posterior
  real lprior = 0;
  theta1 = theta[1];
  theta2 = theta[2];
  lprior += student_t_lpdf(Intercept_mu1 | 3, 0.2, 2.5);
  lprior += student_t_lpdf(sigma1 | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += student_t_lpdf(Intercept_mu2 | 3, 0.2, 2.5);
  lprior += student_t_lpdf(sigma2 | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += dirichlet_lpdf(theta | con_theta);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] mu1 = rep_vector(0.0, N);
    // initialize linear predictor term
    vector[N] mu2 = rep_vector(0.0, N);
    mu1 += Intercept_mu1 + Xc_mu1 * b_mu1;
    mu2 += Intercept_mu2 + Xc_mu2 * b_mu2;
    // likelihood of the mixture model
    for (n in 1:N) {
      array[2] real ps;
      ps[1] = log(theta1) + normal_lpdf(Y[n] | mu1[n], sigma1);
      ps[2] = log(theta2) + normal_lpdf(Y[n] | mu2[n], sigma2);
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
}

