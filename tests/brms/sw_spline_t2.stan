// generated with brms 2.23.0
functions {
}
data {
  int<lower=1> N;  // total number of observations
  vector[N] Y;  // response variable
  // data for splines
  int Ks;  // number of linear effects
  matrix[N, Ks] Xs;  // design matrix for the linear effects
  // data for spline 1
  int nb_1;  // number of bases
  array[nb_1] int knots_1;  // number of knots
  // basis function matrices
  matrix[N, knots_1[1]] Zs_1_1;
  matrix[N, knots_1[2]] Zs_1_2;
  matrix[N, knots_1[3]] Zs_1_3;
  int prior_only;  // should the likelihood be ignored?
}
transformed data {
}
parameters {
  real Intercept;  // temporary intercept for centered predictors
  vector[Ks] bs;  // unpenalized spline coefficients
  // parameters for spline 1
  // standardized penalized spline coefficients
  vector[knots_1[1]] zs_1_1;
  // standardized penalized spline coefficients
  vector[knots_1[2]] zs_1_2;
  // standardized penalized spline coefficients
  vector[knots_1[3]] zs_1_3;
  vector<lower=0>[nb_1] sds_1;  // SDs of penalized spline coefficients
  real<lower=0> sigma;  // dispersion parameter
}
transformed parameters {
  // penalized spline coefficients
  vector[knots_1[1]] s_1_1;
  // penalized spline coefficients
  vector[knots_1[2]] s_1_2;
  // penalized spline coefficients
  vector[knots_1[3]] s_1_3;
  // prior contributions to the log posterior
  real lprior = 0;
  // compute penalized spline coefficients
  s_1_1 = sds_1[1] * zs_1_1;
  // compute penalized spline coefficients
  s_1_2 = sds_1[2] * zs_1_2;
  // compute penalized spline coefficients
  s_1_3 = sds_1[3] * zs_1_3;
  lprior += student_t_lpdf(Intercept | 3, 0.2, 2.5);
  lprior += student_t_lpdf(sds_1 | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += student_t_lpdf(sigma | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] mu = rep_vector(0.0, N);
    mu += Intercept + Xs * bs + Zs_1_1 * s_1_1 + Zs_1_2 * s_1_2 + Zs_1_3 * s_1_3;
    target += normal_lpdf(Y | mu, sigma);
  }
  // priors including constants
  target += lprior;
  target += std_normal_lpdf(zs_1_1);
  target += std_normal_lpdf(zs_1_2);
  target += std_normal_lpdf(zs_1_3);
}
generated quantities {
  // actual population-level intercept
  real b_Intercept = Intercept;
}

