// generated with brms 2.23.0
functions {
  /* Wiener diffusion log-PDF for a single response
   * Args:
   *   y: reaction time data
   *   dec: decision data (0 or 1)
   *   alpha: boundary separation parameter > 0
   *   tau: non-decision time parameter > 0
   *   beta: initial bias parameter in [0, 1]
   *   delta: drift rate parameter
   * Returns:
   *   a scalar to be added to the log posterior
   */
   real wiener_diffusion_lpdf(real y, int dec, real alpha,
                              real tau, real beta, real delta) {
     if (dec == 1) {
       return wiener_lpdf(y | alpha, tau, beta, delta);
     } else {
       return wiener_lpdf(y | alpha, tau, 1 - beta, - delta);
     }
   }
}
data {
  int<lower=1> N;  // total number of observations
  vector[N] Y;  // response variable
  array[N] int<lower=0,upper=1> dec;  // decisions
  int<lower=1> K;  // number of population-level effects
  matrix[N, K] X;  // population-level design matrix
  int<lower=1> Kc;  // number of population-level effects after centering
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
  real min_Y = min(Y);
  matrix[N, Kc] Xc;  // centered version of X without an intercept
  vector[Kc] means_X;  // column means of X before centering
  for (i in 2:K) {
    means_X[i - 1] = mean(X[, i]);
    Xc[, i - 1] = X[, i] - means_X[i - 1];
  }
}
parameters {
  vector[Kc] b;  // regression coefficients
  real Intercept;  // temporary intercept for centered predictors
  real<lower=0> bs;  // boundary separation parameter
  real<lower=0,upper=min_Y> ndt;  // non-decision time parameter
  real<lower=0,upper=1> bias;  // initial bias parameter
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept | 3, 1, 2.5);
  lprior += gamma_lpdf(bs | 1, 1);
  lprior += uniform_lpdf(ndt | 0, min_Y)
    - 1 * log_diff_exp(uniform_lcdf(min_Y | 0, min_Y), uniform_lcdf(0 | 0, min_Y));
  lprior += beta_lpdf(bias | 1, 1);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] mu = rep_vector(0.0, N);
    mu += Intercept + Xc * b;
    for (n in 1:N) {
      target += wiener_diffusion_lpdf(Y[n] | dec[n], bs, ndt, bias, mu[n]);
    }
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_Intercept = Intercept - dot_product(means_X, b);
}

