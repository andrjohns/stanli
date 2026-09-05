// generated with brms 2.23.0
functions {
  /* cumulative-cauchit log-PDF for a single response
   * Args:
   *   y: response category
   *   mu: latent mean parameter
   *   disc: discrimination parameter
   *   thres: ordinal thresholds
   * Returns:
   *   a scalar to be added to the log posterior
   */
   real cumulative_cauchit_lpmf(int y, real mu, real disc, vector thres) {
     int nthres = num_elements(thres);
     real p;
     if (y == 1) {
       p = inv_cauchit(disc * (thres[1] - mu));
     } else if (y == nthres + 1) {
       p = 1 - inv_cauchit(disc * (thres[nthres] - mu));
     } else {
       p = inv_cauchit(disc * (thres[y] - mu)) -
           inv_cauchit(disc * (thres[y - 1] - mu));
     }
     return log(p);
   }
  /* cumulative-cauchit log-PDF for a single response and merged thresholds
   * Args:
   *   y: response category
   *   mu: latent mean parameter
   *   disc: discrimination parameter
   *   thres: vector of merged ordinal thresholds
   *   j: start and end index for the applid threshold within 'thres'
   * Returns:
   *   a scalar to be added to the log posterior
   */
   real cumulative_cauchit_merged_lpmf(int y, real mu, real disc, vector thres, array[] int j) {
     return cumulative_cauchit_lpmf(y | mu, disc, thres[j[1]:j[2]]);
   }

  /* compute the cauchit link
   * Args:
   *   p: a scalar in (0, 1)
   * Returns:
   *   a scalar in (-Inf, Inf)
   */
   real cauchit(real p) {
     return tan(pi() * (p - 0.5));
   }
  /* compute the cauchit link (vectorized)
   * Args:
   *   p: a vector in (0, 1)
   * Returns:
   *   a vector in (-Inf, Inf)
   */
   vector cauchit(vector p) {
     return tan(pi() * (p - 0.5));
   }
  /* compute the inverse of the cauchit link
   * Args:
   *   y: a scalar in (-Inf, Inf)
   * Returns:
   *   a scalar in (0, 1)
   */
   real inv_cauchit(real y) {
     return atan(y) / pi() + 0.5;
   }
  /* compute the inverse of the cauchit link (vectorized)
   * Args:
   *   y: a vector in (-Inf, Inf)
   * Returns:
   *   a vector in (0, 1)
   */
   vector inv_cauchit(vector y) {
     return atan(y) / pi() + 0.5;
   }
}
data {
  int<lower=1> N;  // total number of observations
  array[N] int Y;  // response variable
  int<lower=2> nthres;  // number of thresholds
  int<lower=1> K;  // number of population-level effects
  matrix[N, K] X;  // population-level design matrix
  int<lower=1> Kc;  // number of population-level effects after centering
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
  ordered[nthres] Intercept;  // temporary thresholds for centered predictors
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
    // initialize linear predictor term
    vector[N] mu = rep_vector(0.0, N);
    mu += Xc * b;
    for (n in 1:N) {
      target += cumulative_cauchit_lpmf(Y[n] | mu[n], disc, Intercept);
    }
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // compute actual thresholds
  vector[nthres] b_Intercept = Intercept + dot_product(means_X, b);
}

