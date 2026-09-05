// generated with brms 2.23.0
functions {
}
data {
  int<lower=1> N;  // total number of observations
  vector[N] Y;  // response variable
  int<lower=1> Ksp;  // number of special effects terms
  // data for noise-free variables
  int<lower=1> Mme_1;  // number of groups
  vector[N] Xn_1;  // noisy values
  vector<lower=0>[N] noise_1;  // measurement noise
  vector[N] Xn_2;  // noisy values
  vector<lower=0>[N] noise_2;  // measurement noise
  int<lower=1> NCme_1;  // number of latent correlations
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
}
parameters {
  real Intercept;  // temporary intercept for centered predictors
  vector[Ksp] bsp;  // special effects coefficients
  real<lower=0> sigma;  // dispersion parameter
  // parameters for noise free variables
  vector[Mme_1] meanme_1;  // latent means
  vector<lower=0>[Mme_1] sdme_1;  // latent SDs
  matrix[Mme_1, N] zme_1;  // standardized latent values
  cholesky_factor_corr[Mme_1] Lme_1;  // cholesky factor of the latent correlation matrix
}
transformed parameters {
  matrix[N, Mme_1] Xme1;  // actual latent values
  // using separate vectors increases efficiency
  vector[N] Xme_1;
  // using separate vectors increases efficiency
  vector[N] Xme_2;
  // prior contributions to the log posterior
  real lprior = 0;
  // compute actual latent values
  Xme1 = rep_matrix(transpose(meanme_1), N) + transpose(diag_pre_multiply(sdme_1, Lme_1) * zme_1);
  Xme_1 = Xme1[, 1];
  Xme_2 = Xme1[, 2];
  lprior += student_t_lpdf(Intercept | 3, -0.3, 2.5);
  lprior += student_t_lpdf(sigma | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += lkj_corr_cholesky_lpdf(Lme_1 | 1);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] mu = rep_vector(0.0, N);
    mu += Intercept;
    for (n in 1:N) {
      // add more terms to the linear predictor
      mu[n] += (bsp[1]) * Xme_1[n] + (bsp[2]) * Xme_2[n];
    }
    target += normal_lpdf(Y | mu, sigma);
  }
  // priors including constants
  target += lprior;
  target += normal_lpdf(Xn_1 | Xme_1, noise_1);
  target += normal_lpdf(Xn_2 | Xme_2, noise_2);
  target += std_normal_lpdf(to_vector(zme_1));
}
generated quantities {
  // actual population-level intercept
  real b_Intercept = Intercept;
  // obtain latent correlation matrix
  corr_matrix[Mme_1] Corme_1 = multiply_lower_tri_self_transpose(Lme_1);
  vector<lower=-1,upper=1>[NCme_1] corme_1;
  // extract upper diagonal of correlation matrix
  for (k in 1:Mme_1) {
    for (j in 1:(k - 1)) {
      corme_1[choose(k - 1, 2) + j] = Corme_1[j, k];
    }
  }
}

