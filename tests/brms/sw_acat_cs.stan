// generated with brms 2.23.0
functions {
  /* acat-logit log-PDF for a single response
   * Args:
   *   y: response category
   *   mu: latent mean parameter
   *   disc: discrimination parameter
   *   thres: ordinal thresholds
   * Returns:
   *   a scalar to be added to the log posterior
   */
   real acat_logit_lpmf(int y, real mu, real disc, vector thres) {
     int nthres = num_elements(thres);
     vector[nthres + 1] p = append_row(0, cumulative_sum(disc * (mu - thres)));
     return p[y] - log_sum_exp(p);
   }
  /* acat-logit log-PDF for a single response and merged thresholds
   * Args:
   *   y: response category
   *   mu: latent mean parameter
   *   disc: discrimination parameter
   *   thres: vector of merged ordinal thresholds
   *   j: start and end index for the applid threshold within 'thres'
   * Returns:
   *   a scalar to be added to the log posterior
   */
   real acat_logit_merged_lpmf(int y, real mu, real disc, vector thres, array[] int j) {
     return acat_logit_lpmf(y | mu, disc, thres[j[1]:j[2]]);
   }

}
data {
  int<lower=1> N;  // total number of observations
  array[N] int Y;  // response variable
  int<lower=2> nthres;  // number of thresholds
  int<lower=1> K;  // number of population-level effects
  matrix[N, K] X;  // population-level design matrix
  int<lower=1> Kc;  // number of population-level effects after centering
  int<lower=1> Kcs;  // number of category specific effects
  matrix[N, Kcs] Xcs;  // category specific design matrix
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
  matrix[N, Kc] Xc;  // centered version of X
  vector[Kc] means_X;  // column means of X before centering
  for (i in 1:K) {
    means_X[i] = mean(X[, i]);
    Xc[, i] = X[, i] - means_X[i];
  }
}
parameters {
  vector[Kc] b;  // regression coefficients
  vector[nthres] Intercept;  // temporary thresholds for centered predictors
  matrix[Kcs, nthres] bcs;  // category specific effects
}
transformed parameters {
  real disc = 1;  // discrimination parameters
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept | 3, 0, 2.5);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // linear predictor for category specific effects
    matrix[N, nthres] mucs = Xcs * bcs;
    // initialize linear predictor term
    vector[N] mu = rep_vector(0.0, N);
    mu += Xc * b;
    for (n in 1:N) {
      target += acat_logit_lpmf(Y[n] | mu[n], disc, Intercept - transpose(mucs[n]));
    }
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // compute actual thresholds
  vector[nthres] b_Intercept = Intercept + dot_product(means_X, b);
}

