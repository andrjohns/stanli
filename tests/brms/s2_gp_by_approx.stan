// generated with brms 2.23.0
functions {
  /* Spectral density function of a Gaussian process
   * with squared exponential covariance kernel
   * Args:
   *   x: array of numeric values of dimension NB x D
   *   sdgp: marginal SD parameter
   *   lscale: vector of length-scale parameters
   * Returns:
   *   numeric vector of length NB of the SPD evaluated at 'x'
   */
  vector spd_gp_exp_quad(data array[] vector x, real sdgp, vector lscale) {
    int NB = dims(x)[1];
    int D = dims(x)[2];
    int Dls = rows(lscale);
    real constant = square(sdgp) * sqrt(2 * pi())^D;
    vector[NB] out;
    if (Dls == 1) {
      // one dimensional or isotropic GP
      real neg_half_lscale2 = -0.5 * square(lscale[1]);
      constant = constant * lscale[1]^D;
      for (m in 1:NB) {
        out[m] = constant * exp(neg_half_lscale2 * dot_self(x[m]));
      }
    } else {
      // multi-dimensional non-isotropic GP
      vector[Dls] neg_half_lscale2 = -0.5 * square(lscale);
      constant = constant * prod(lscale);
      for (m in 1:NB) {
        out[m] = constant * exp(dot_product(neg_half_lscale2, square(x[m])));
      }
    }
    return out;
  }
}
data {
  int<lower=1> N;  // total number of observations
  vector[N] Y;  // response variable
  // data related to GPs
  int<lower=1> Kgp_1;  // number of sub-GPs (equal to 1 unless 'by' was used)
  int<lower=1> Dgp_1;  // GP dimension
  // number of basis functions of an approximate GP
  int<lower=1> NBgp_1;
  // number of observations relevant for a certain sub-GP
  array[Kgp_1] int<lower=1> Ngp_1;
  // indices and contrasts of sub-GPs per observation
  array[Ngp_1[1]] int<lower=1> Igp_1_1;
  vector[Ngp_1[1]] Cgp_1_1;
  array[Ngp_1[2]] int<lower=1> Igp_1_2;
  vector[Ngp_1[2]] Cgp_1_2;
  // number of latent GP groups
  array[Kgp_1] int<lower=1> Nsubgp_1;
  // indices of latent GP groups per observation
  array[Ngp_1[1]] int<lower=1> Jgp_1_1;
  // indices of latent GP groups per observation
  array[Ngp_1[2]] int<lower=1> Jgp_1_2;
  // approximate GP basis matrices and eigenvalues
  matrix[Nsubgp_1[1], NBgp_1] Xgp_1_1;
  array[NBgp_1] vector[Dgp_1] slambda_1_1;
  matrix[Nsubgp_1[2], NBgp_1] Xgp_1_2;
  array[NBgp_1] vector[Dgp_1] slambda_1_2;
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
}
parameters {
  real Intercept;  // temporary intercept for centered predictors
  vector<lower=0>[Kgp_1] sdgp_1;  // GP standard deviation parameters
  array[Kgp_1] vector<lower=0>[1] lscale_1;  // GP length-scale parameters
  // latent variables of the GP
  vector[NBgp_1] zgp_1_1;
  vector[NBgp_1] zgp_1_2;
  real<lower=0> sigma;  // dispersion parameter
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept | 3, -0.3, 2.5);
  lprior += student_t_lpdf(sdgp_1 | 3, 0, 2.5)
    - 2 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += inv_gamma_lpdf(lscale_1[1][1] | 1.494197, 0.056607);
  lprior += inv_gamma_lpdf(lscale_1[2][1] | 1.494197, 0.056607);
  lprior += student_t_lpdf(sigma | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // scale latent variables of the GP
    vector[NBgp_1] rgp_1_1 = sqrt(spd_gp_exp_quad(slambda_1_1, sdgp_1[1], lscale_1[1])) .* zgp_1_1;
    vector[NBgp_1] rgp_1_2 = sqrt(spd_gp_exp_quad(slambda_1_2, sdgp_1[2], lscale_1[2])) .* zgp_1_2;
    vector[Nsubgp_1[1]] gp_pred_1_1 = Xgp_1_1 * rgp_1_1;
    vector[Nsubgp_1[2]] gp_pred_1_2 = Xgp_1_2 * rgp_1_2;
    // initialize linear predictor term
    vector[N] mu = rep_vector(0.0, N);
    mu[Igp_1_1] += Cgp_1_1 .* gp_pred_1_1[Jgp_1_1];
    mu[Igp_1_2] += Cgp_1_2 .* gp_pred_1_2[Jgp_1_2];
    mu += Intercept;
    target += normal_lpdf(Y | mu, sigma);
  }
  // priors including constants
  target += lprior;
  target += std_normal_lpdf(zgp_1_1);
  target += std_normal_lpdf(zgp_1_2);
}
generated quantities {
  // actual population-level intercept
  real b_Intercept = Intercept;
}

