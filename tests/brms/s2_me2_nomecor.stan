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
  vector[N] zme_1;  // standardized latent values
  vector[N] zme_2;  // standardized latent values
}
transformed parameters {
  vector[N] Xme_1;  // actual latent values
  vector[N] Xme_2;  // actual latent values
  // prior contributions to the log posterior
  real lprior = 0;
  // compute actual latent values
  Xme_1 = meanme_1[1] + sdme_1[1] * zme_1;
  // compute actual latent values
  Xme_2 = meanme_1[2] + sdme_1[2] * zme_2;
  lprior += student_t_lpdf(Intercept | 3, -0.3, 2.5);
  lprior += student_t_lpdf(sigma | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
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
  target += std_normal_lpdf(zme_1);
  target += std_normal_lpdf(zme_2);
}
generated quantities {
  // actual population-level intercept
  real b_Intercept = Intercept;
}

