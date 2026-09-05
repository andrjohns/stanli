// generated with brms 2.23.0
functions {
  /* compute a latent Gaussian process with squared exponential kernel
   * Args:
   *   x: array of continuous predictor values
   *   sdgp: marginal SD parameter
   *   lscale: length-scale parameter
   *   zgp: vector of independent standard normal variables
   * Returns:
   *   a vector to be added to the linear predictor
   */
  vector gp_exp_quad(data array[] vector x, real sdgp, vector lscale, vector zgp) {
    int Dls = rows(lscale);
    int N = size(x);
    matrix[N, N] cov;
    if (Dls == 1) {
      // one dimensional or isotropic GP
      cov = gp_exp_quad_cov(x, sdgp, lscale[1]);
    } else {
      // multi-dimensional non-isotropic GP
      cov = gp_exp_quad_cov(x[, 1], sdgp, lscale[1]);
      for (d in 2:Dls) {
        cov = cov .* gp_exp_quad_cov(x[, d], 1, lscale[d]);
      }
    }
    for (n in 1:N) {
      // deal with numerical non-positive-definiteness
      cov[n, n] += 1e-12;
    }
    return cholesky_decompose(cov) * zgp;
  }
}
data {
  int<lower=1> N;  // total number of observations
  vector[N] Y;  // response variable
  // data related to GPs
  int<lower=1> Kgp_1;  // number of sub-GPs (equal to 1 unless 'by' was used)
  int<lower=1> Dgp_1;  // GP dimension
  // number of observations relevant for a certain sub-GP
  array[Kgp_1] int<lower=1> Ngp_1;
  // indices and contrasts of sub-GPs per observation
  array[Ngp_1[1]] int<lower=1> Igp_1_1;
  vector[Ngp_1[1]] Cgp_1_1;
  array[Ngp_1[2]] int<lower=1> Igp_1_2;
  vector[Ngp_1[2]] Cgp_1_2;
  array[Ngp_1[3]] int<lower=1> Igp_1_3;
  vector[Ngp_1[3]] Cgp_1_3;
  array[Ngp_1[4]] int<lower=1> Igp_1_4;
  vector[Ngp_1[4]] Cgp_1_4;
  array[Ngp_1[5]] int<lower=1> Igp_1_5;
  vector[Ngp_1[5]] Cgp_1_5;
  // number of latent GP groups
  array[Kgp_1] int<lower=1> Nsubgp_1;
  // indices of latent GP groups per observation
  array[Ngp_1[1]] int<lower=1> Jgp_1_1;
  // indices of latent GP groups per observation
  array[Ngp_1[2]] int<lower=1> Jgp_1_2;
  // indices of latent GP groups per observation
  array[Ngp_1[3]] int<lower=1> Jgp_1_3;
  // indices of latent GP groups per observation
  array[Ngp_1[4]] int<lower=1> Jgp_1_4;
  // indices of latent GP groups per observation
  array[Ngp_1[5]] int<lower=1> Jgp_1_5;
  // covariates of the GP
  array[Nsubgp_1[1]] vector[Dgp_1] Xgp_1_1;
  array[Nsubgp_1[2]] vector[Dgp_1] Xgp_1_2;
  array[Nsubgp_1[3]] vector[Dgp_1] Xgp_1_3;
  array[Nsubgp_1[4]] vector[Dgp_1] Xgp_1_4;
  array[Nsubgp_1[5]] vector[Dgp_1] Xgp_1_5;
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
}
parameters {
  real Intercept;  // temporary intercept for centered predictors
  vector<lower=0>[Kgp_1] sdgp_1;  // GP standard deviation parameters
  array[Kgp_1] vector<lower=0>[1] lscale_1;  // GP length-scale parameters
  // latent variables of the GP
  vector[Nsubgp_1[1]] zgp_1_1;
  vector[Nsubgp_1[2]] zgp_1_2;
  vector[Nsubgp_1[3]] zgp_1_3;
  vector[Nsubgp_1[4]] zgp_1_4;
  vector[Nsubgp_1[5]] zgp_1_5;
  real<lower=0> sigma;  // dispersion parameter
}
transformed parameters {
  // prior contributions to the log posterior
  real lprior = 0;
  lprior += student_t_lpdf(Intercept | 3, -0.3, 2.5);
  lprior += student_t_lpdf(sdgp_1 | 3, 0, 2.5)
    - 5 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += inv_gamma_lpdf(lscale_1[1][1] | 1.896808, 0.126425);
  lprior += inv_gamma_lpdf(lscale_1[2][1] | 1.494197, 0.056607);
  lprior += inv_gamma_lpdf(lscale_1[3][1] | 2.118736, 0.175987);
  lprior += inv_gamma_lpdf(lscale_1[4][1] | 1.494197, 0.056607);
  lprior += inv_gamma_lpdf(lscale_1[5][1] | 1.494197, 0.056607);
  lprior += student_t_lpdf(sigma | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
}
model {
  // likelihood including constants
  if (!prior_only) {
    vector[Nsubgp_1[1]] gp_pred_1_1 = gp_exp_quad(Xgp_1_1, sdgp_1[1], lscale_1[1], zgp_1_1);
    vector[Nsubgp_1[2]] gp_pred_1_2 = gp_exp_quad(Xgp_1_2, sdgp_1[2], lscale_1[2], zgp_1_2);
    vector[Nsubgp_1[3]] gp_pred_1_3 = gp_exp_quad(Xgp_1_3, sdgp_1[3], lscale_1[3], zgp_1_3);
    vector[Nsubgp_1[4]] gp_pred_1_4 = gp_exp_quad(Xgp_1_4, sdgp_1[4], lscale_1[4], zgp_1_4);
    vector[Nsubgp_1[5]] gp_pred_1_5 = gp_exp_quad(Xgp_1_5, sdgp_1[5], lscale_1[5], zgp_1_5);
    // initialize linear predictor term
    vector[N] mu = rep_vector(0.0, N);
    mu[Igp_1_1] += Cgp_1_1 .* gp_pred_1_1[Jgp_1_1];
    mu[Igp_1_2] += Cgp_1_2 .* gp_pred_1_2[Jgp_1_2];
    mu[Igp_1_3] += Cgp_1_3 .* gp_pred_1_3[Jgp_1_3];
    mu[Igp_1_4] += Cgp_1_4 .* gp_pred_1_4[Jgp_1_4];
    mu[Igp_1_5] += Cgp_1_5 .* gp_pred_1_5[Jgp_1_5];
    mu += Intercept;
    target += normal_lpdf(Y | mu, sigma);
  }
  // priors including constants
  target += lprior;
  target += std_normal_lpdf(zgp_1_1);
  target += std_normal_lpdf(zgp_1_2);
  target += std_normal_lpdf(zgp_1_3);
  target += std_normal_lpdf(zgp_1_4);
  target += std_normal_lpdf(zgp_1_5);
}
generated quantities {
  // actual population-level intercept
  real b_Intercept = Intercept;
}

