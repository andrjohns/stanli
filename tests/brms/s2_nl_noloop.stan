// generated with brms 2.23.0
functions {
}
data {
  int<lower=1> N;  // total number of observations
  vector[N] Y;  // response variable
  int<lower=1> K_a;  // number of population-level effects
  matrix[N, K_a] X_a;  // population-level design matrix
  int<lower=1> K_b;  // number of population-level effects
  matrix[N, K_b] X_b;  // population-level design matrix
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
}
parameters {
  vector[K_a] b_a;  // regression coefficients
  vector[K_b] b_b;  // regression coefficients
  real<lower=0> sigma;  // dispersion parameter
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += normal_lpdf(b_a | 0, 1);
  lprior += normal_lpdf(b_b | 0, 1);
  lprior += student_t_lpdf(sigma | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] nlp_a = rep_vector(0.0, N);
    // initialize linear predictor term
    vector[N] nlp_b = rep_vector(0.0, N);
    // initialize non-linear predictor term
    vector[N] mu;
    nlp_a += X_a * b_a;
    nlp_b += X_b * b_b;
    // compute non-linear predictor values
    mu = (exp(nlp_a) - nlp_b);
    target += normal_lpdf(Y | mu, sigma);
  }
  // priors including constants
  target += lprior;
}
generated quantities {
}

