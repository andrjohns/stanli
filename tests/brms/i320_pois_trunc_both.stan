// generated with brms 2.23.0
functions {
}
data {
  int<lower=1> N;  // total number of observations
  array[N] int Y;  // response variable
  array[N] int lb;  // lower truncation bounds;
  array[N] int ub;  // upper truncation bounds
  int<lower=1> K;  // number of population-level effects
  matrix[N, K] X;  // population-level design matrix
  int<lower=1> Kc;  // number of population-level effects after centering
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
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
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept | 3, 1.4, 2.5);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] mu = rep_vector(0.0, N);
    mu += Intercept + Xc * b;
    mu = exp(mu);
    for (n in 1:N) {
      target += poisson_lpmf(Y[n] | mu[n]) -
        log_diff_exp(poisson_lcdf(ub[n] | mu[n]), poisson_lcdf(lb[n] - 1 | mu[n]));
    }
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_Intercept = Intercept - dot_product(means_X, b);
}

