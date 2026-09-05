// generated with brms 2.23.0
functions {
}
data {
  int<lower=1> N;  // total number of observations
  vector[N] Y;  // response variable
  // censoring indicator: 0 = event, 1 = right, -1 = left, 2 = interval censored
  array[N] int<lower=-1,upper=2> cens;
  int<lower=1> K;  // number of population-level effects
  matrix[N, K] X;  // population-level design matrix
  int<lower=1> Kc;  // number of population-level effects after centering
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
  // indices of censored data
  int Nevent = 0;
  int Nrcens = 0;
  int Nlcens = 0;
  array[N] int Jevent;
  array[N] int Jrcens;
  array[N] int Jlcens;
  matrix[N, Kc] Xc;  // centered version of X without an intercept
  vector[Kc] means_X;  // column means of X before centering
  // collect indices of censored data
  for (n in 1:N) {
    if (cens[n] == 0) {
      Nevent += 1;
      Jevent[Nevent] = n;
    } else if (cens[n] == 1) {
      Nrcens += 1;
      Jrcens[Nrcens] = n;
    } else if (cens[n] == -1) {
      Nlcens += 1;
      Jlcens[Nlcens] = n;
    }
  }
  for (i in 2:K) {
    means_X[i - 1] = mean(X[, i]);
    Xc[, i - 1] = X[, i] - means_X[i - 1];
  }
}
parameters {
  vector[Kc] b;  // regression coefficients
  real Intercept;  // temporary intercept for centered predictors
  real<lower=0> sigma;  // dispersion parameter
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept | 3, 0.8, 2.5);
  lprior += student_t_lpdf(sigma | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] mu = rep_vector(0.0, N);
    mu += Intercept + Xc * b;
    // vectorized log-likelihood contributions of censored data
    target += normal_lpdf(Y[Jevent[1:Nevent]] | mu[Jevent[1:Nevent]], sigma);
    target += normal_lccdf(Y[Jrcens[1:Nrcens]] | mu[Jrcens[1:Nrcens]], sigma);
    target += normal_lcdf(Y[Jlcens[1:Nlcens]] | mu[Jlcens[1:Nlcens]], sigma);
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_Intercept = Intercept - dot_product(means_X, b);
}

