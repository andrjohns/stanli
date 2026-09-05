// generated with brms 2.23.0
functions {
}
data {
  int<lower=1> N;  // total number of observations
  vector[N] Y;  // response variable
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
  real<lower=0> sigma;  // dispersion parameter
  real alpha;  // skewness parameter
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept | 3, 0.2, 2.5);
  lprior += student_t_lpdf(sigma | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += normal_lpdf(alpha | 0, 4);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] mu = rep_vector(0.0, N);
    // parameters used to transform the skew-normal distribution
    real delta;  // transformed alpha parameter
    real omega;  // scale parameter
    mu += Intercept + Xc * b;
    // use efficient skew-normal parameterization
    delta = alpha / sqrt(1 + alpha^2);
    omega = sigma / sqrt(1 - sqrt(2 / pi())^2 * delta^2);
    for (n in 1:N) {
      mu[n] = mu[n] - omega * delta * sqrt(2 / pi());
    }
    target += skew_normal_lpdf(Y | mu, omega, alpha);
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_Intercept = Intercept - dot_product(means_X, b);
}

