// generated with brms 2.23.0
functions {
  /* distribution functions of the Cox proportional hazards model
   * parameterize hazard(t) = baseline(t) * mu
   * so that higher values of 'mu' imply lower survival times
   * Args:
   *   y: the response value; currently ignored as the relevant
   *     information is passed via 'bhaz' and 'cbhaz'
   *   mu: positive location parameter
   *   bhaz: baseline hazard
   *   cbhaz: cumulative baseline hazard
   */
  real cox_lhaz(real y, real mu, real bhaz, real cbhaz) {
    return log(bhaz) + log(mu);
  }
  vector cox_lhaz(vector y, vector mu, vector bhaz, vector cbhaz) {
    return log(bhaz) + log(mu);
  }

  // equivalent to the log survival function
  real cox_lccdf(real y, real mu, real bhaz, real cbhaz) {
    return - cbhaz * mu;
  }
  real cox_lccdf(vector y, vector mu, vector bhaz, vector cbhaz) {
    return - dot_product(cbhaz, mu);
  }

  real cox_lcdf(real y, real mu, real bhaz, real cbhaz) {
    return log1m_exp(cox_lccdf(y | mu, bhaz, cbhaz));
  }
  real cox_lcdf(vector y, vector mu, vector bhaz, vector cbhaz) {
    return sum(log1m_exp(- cbhaz .* mu));
  }

  real cox_lpdf(real y, real mu, real bhaz, real cbhaz) {
    return cox_lhaz(y, mu, bhaz, cbhaz) + cox_lccdf(y | mu, bhaz, cbhaz);
  }
  real cox_lpdf(vector y, vector mu, vector bhaz, vector cbhaz) {
    return sum(cox_lhaz(y, mu, bhaz, cbhaz)) + cox_lccdf(y | mu, bhaz, cbhaz);
  }

  // Distribution functions of the Cox model in log parameterization
  real cox_log_lhaz(real y, real log_mu, real bhaz, real cbhaz) {
    return log(bhaz) + log_mu;
  }
  vector cox_log_lhaz(vector y, vector log_mu, vector bhaz, vector cbhaz) {
    return log(bhaz) + log_mu;
  }

  real cox_log_lccdf(real y, real log_mu, real bhaz, real cbhaz) {
    return - cbhaz * exp(log_mu);
  }
  real cox_log_lccdf(vector y, vector log_mu, vector bhaz, vector cbhaz) {
    return - dot_product(cbhaz, exp(log_mu));
  }

  real cox_log_lcdf(real y, real log_mu, real bhaz, real cbhaz) {
    return log1m_exp(cox_log_lccdf(y | log_mu, bhaz, cbhaz));
  }
  real cox_log_lcdf(vector y, vector log_mu, vector bhaz, vector cbhaz) {
    return sum(log1m_exp(- cbhaz .* exp(log_mu)));
  }

  real cox_log_lpdf(real y, real log_mu, real bhaz, real cbhaz) {
    return cox_log_lhaz(y, log_mu, bhaz, cbhaz) +
           cox_log_lccdf(y | log_mu, bhaz, cbhaz);
  }
  real cox_log_lpdf(vector y, vector log_mu, vector bhaz, vector cbhaz) {
    return sum(cox_log_lhaz(y, log_mu, bhaz, cbhaz)) +
           cox_log_lccdf(y | log_mu, bhaz, cbhaz);
  }
}
data {
  int<lower=1> N;  // total number of observations
  vector[N] Y;  // response variable
  // censoring indicator: 0 = event, 1 = right, -1 = left, 2 = interval censored
  array[N] int<lower=-1,upper=2> cens;
  int<lower=1> K;  // number of population-level effects
  matrix[N, K] X;  // population-level design matrix
  int<lower=1> Kc;  // number of population-level effects after centering
  // data for flexible baseline functions
  int Kbhaz;  // number of basis functions
  // design matrix of the baseline function
  matrix[N, Kbhaz] Zbhaz;
  // design matrix of the cumulative baseline function
  matrix[N, Kbhaz] Zcbhaz;
  // a-priori concentration vector of baseline coefficients
  vector<lower=0>[Kbhaz] con_sbhaz;
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
  // baseline hazard coefficients
  simplex[Kbhaz] sbhaz;
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept | 3, -0.5, 2.5);
  lprior += dirichlet_lpdf(sbhaz | con_sbhaz);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // compute values of baseline function
    vector[N] bhaz = Zbhaz * sbhaz;
    // compute values of cumulative baseline function
    vector[N] cbhaz = Zcbhaz * sbhaz;
    // initialize linear predictor term
    vector[N] mu = rep_vector(0.0, N);
    mu += Intercept + Xc * b;
    // vectorized log-likelihood contributions of censored data
    target += cox_log_lpdf(Y[Jevent[1:Nevent]] | mu[Jevent[1:Nevent]], bhaz[Jevent[1:Nevent]], cbhaz[Jevent[1:Nevent]]);
    target += cox_log_lccdf(Y[Jrcens[1:Nrcens]] | mu[Jrcens[1:Nrcens]], bhaz[Jrcens[1:Nrcens]], cbhaz[Jrcens[1:Nrcens]]);
    target += cox_log_lcdf(Y[Jlcens[1:Nlcens]] | mu[Jlcens[1:Nlcens]], bhaz[Jlcens[1:Nlcens]], cbhaz[Jlcens[1:Nlcens]]);
  }
  // priors including constants
  target += lprior;
}
generated quantities {
  // actual population-level intercept
  real b_Intercept = Intercept - dot_product(means_X, b);
}

