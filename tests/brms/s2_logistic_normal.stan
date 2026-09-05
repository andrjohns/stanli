// generated with brms 2.23.0
functions {
  /* multi-logit transform
   * Args:
   *   y: simplex vector of length D
   *   ref: a single integer in 1:D indicating the reference category
   * Returns:
   *   an unbounded real vector of length D - 1
   */
   vector multi_logit(vector y, int ref) {
     vector[rows(y) - 1] x;
     for (i in 1:(ref - 1)) {
       x[i] = log(y[i]) - log(y[ref]);
     }
      for (i in (ref+1):rows(y)) {
       x[i - 1] = log(y[i]) - log(y[ref]);
     }
     return(x);
   }
  /* logistic-normal log-PDF
   * Args:
   *   y: simplex vector of response values (length D)
   *   mu: vector of means on the logit scale (length D-1)
   *   sigma: vector for standard deviations on the logit scale (length D-1)
   *   Lcor: Cholesky correlation matrix on the logit scale (dim D-1)
   *   ref: a single integer in 1:D indicating the reference category
   * Returns:
   *   a scalar to be added to the log posterior
   */
   real logistic_normal_cholesky_cor_lpdf(vector y, vector mu, vector sigma,
                                          matrix Lcor, int ref) {
     int D = rows(y);
     vector[D - 1] x = multi_logit(y, ref);
     matrix[D - 1, D - 1] Lcov = diag_pre_multiply(sigma, Lcor);
     // multi-normal plus Jacobian adjustment of multivariate logit transform
     return multi_normal_cholesky_lpdf(x | mu, Lcov) - sum(log(y));
   }
}
data {
  int<lower=1> N;  // total number of observations
  int<lower=2> ncat;  // number of categories
  array[N] vector[ncat] Y;  // response array
  int<lower=1> K_mus2;  // number of population-level effects
  matrix[N, K_mus2] X_mus2;  // population-level design matrix
  int<lower=1> Kc_mus2;  // number of population-level effects after centering
  int<lower=1> K_mus3;  // number of population-level effects
  matrix[N, K_mus3] X_mus3;  // population-level design matrix
  int<lower=1> Kc_mus3;  // number of population-level effects after centering
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
  matrix[N, Kc_mus2] Xc_mus2;  // centered version of X_mus2 without an intercept
  vector[Kc_mus2] means_X_mus2;  // column means of X_mus2 before centering
  matrix[N, Kc_mus3] Xc_mus3;  // centered version of X_mus3 without an intercept
  vector[Kc_mus3] means_X_mus3;  // column means of X_mus3 before centering
  // number of logistic normal correlations
  int nlncor = choose(ncat-1, 2);
  for (i in 2:K_mus2) {
    means_X_mus2[i - 1] = mean(X_mus2[, i]);
    Xc_mus2[, i - 1] = X_mus2[, i] - means_X_mus2[i - 1];
  }
  for (i in 2:K_mus3) {
    means_X_mus3[i - 1] = mean(X_mus3[, i]);
    Xc_mus3[, i - 1] = X_mus3[, i] - means_X_mus3[i - 1];
  }
}
parameters {
  vector[Kc_mus2] b_mus2;  // regression coefficients
  real Intercept_mus2;  // temporary intercept for centered predictors
  vector[Kc_mus3] b_mus3;  // regression coefficients
  real Intercept_mus3;  // temporary intercept for centered predictors
  real<lower=0> sigmas2;  // dispersion parameter
  real<lower=0> sigmas3;  // dispersion parameter
  cholesky_factor_corr[ncat-1] Llncor;  // logistic-normal Cholesky correlation matrix
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept_mus2 | 3, 0, 2.5);
  lprior += student_t_lpdf(Intercept_mus3 | 3, 0, 2.5);
  lprior += student_t_lpdf(sigmas2 | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += student_t_lpdf(sigmas3 | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += lkj_corr_cholesky_lpdf(Llncor | 1);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] mus2 = rep_vector(0.0, N);
    // initialize linear predictor term
    vector[N] mus3 = rep_vector(0.0, N);
    // linear predictor matrix
    array[N] vector[ncat-1] mu;
    // sigma parameter vector
    vector[ncat-1] sigma = transpose([sigmas2, sigmas3]);
    mus2 += Intercept_mus2 + Xc_mus2 * b_mus2;
    mus3 += Intercept_mus3 + Xc_mus3 * b_mus3;
    for (n in 1:N) {
      mu[n] = transpose([mus2[n], mus3[n]]);
    }
    for (n in 1:N) {
      target += logistic_normal_cholesky_cor_lpdf(Y[n] | mu[n], sigma, Llncor, 1);
    }
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_mus2_Intercept = Intercept_mus2 - dot_product(means_X_mus2, b_mus2);
  // actual population-level intercept
  real b_mus3_Intercept = Intercept_mus3 - dot_product(means_X_mus3, b_mus3);
  // logistic normal correlations
  corr_matrix[ncat-1] Lncor = multiply_lower_tri_self_transpose(Llncor);
  vector<lower=-1,upper=1>[nlncor] lncor;
  // extract upper diagonal of correlation matrix
  for (k in 1:ncat-1) {
    for (j in 1:(k - 1)) {
      lncor[choose(k - 1, 2) + j] = Lncor[j, k];
    }
  }
}

