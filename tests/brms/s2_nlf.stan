// generated with brms 2.23.0
functions {
}
data {
  int<lower=1> N;  // total number of observations
  vector[N] Y;  // response variable
  int<lower=1> K_a2;  // number of population-level effects
  matrix[N, K_a2] X_a2;  // population-level design matrix
  int<lower=1> K_l1;  // number of population-level effects
  matrix[N, K_l1] X_l1;  // population-level design matrix
  // covariates for non-linear functions
  vector[N] C_1;
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
}
parameters {
  vector[K_a2] b_a2;  // regression coefficients
  vector[K_l1] b_l1;  // regression coefficients
  real<lower=0> sigma;  // dispersion parameter
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += normal_lpdf(b_a2 | 0, 1);
  lprior += normal_lpdf(b_l1 | 0, 1);
  lprior += student_t_lpdf(sigma | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] nlp_a2 = rep_vector(0.0, N);
    // initialize linear predictor term
    vector[N] nlp_l1 = rep_vector(0.0, N);
    // initialize non-linear predictor term
    vector[N] nlp_a1;
    // initialize non-linear predictor term
    vector[N] mu;
    nlp_a2 += X_a2 * b_a2;
    nlp_l1 += X_l1 * b_l1;
    for (n in 1:N) {
      // compute non-linear predictor values
      nlp_a1[n] = (exp(nlp_l1[n]));
    }
    for (n in 1:N) {
      // compute non-linear predictor values
      mu[n] = (nlp_a1[n] - nlp_a2[n] ^ C_1[n]);
    }
    target += normal_lpdf(Y | mu, sigma);
  }
  // priors including constants
  target += lprior;
}
generated quantities {
}

