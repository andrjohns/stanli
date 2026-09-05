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
  // data for spline 2
  int nb_2;  // number of bases
  array[nb_2] int knots_2;  // number of knots
  // basis function matrices
  matrix[N, knots_2[1]] Zs_2_1;
  // data for spline 3
  int nb_3;  // number of bases
  array[nb_3] int knots_3;  // number of knots
  // basis function matrices
  matrix[N, knots_3[1]] Zs_3_1;
  // data for spline 4
  int nb_4;  // number of bases
  array[nb_4] int knots_4;  // number of knots
  // basis function matrices
  matrix[N, knots_4[1]] Zs_4_1;
  // data for spline 5
  int nb_5;  // number of bases
  array[nb_5] int knots_5;  // number of knots
  // basis function matrices
  matrix[N, knots_5[1]] Zs_5_1;
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
  vector<lower=0>[nb_1] sds_1;  // SDs of penalized spline coefficients
  // parameters for spline 2
  // standardized penalized spline coefficients
  vector[knots_2[1]] zs_2_1;
  vector<lower=0>[nb_2] sds_2;  // SDs of penalized spline coefficients
  // parameters for spline 3
  // standardized penalized spline coefficients
  vector[knots_3[1]] zs_3_1;
  vector<lower=0>[nb_3] sds_3;  // SDs of penalized spline coefficients
  // parameters for spline 4
  // standardized penalized spline coefficients
  vector[knots_4[1]] zs_4_1;
  vector<lower=0>[nb_4] sds_4;  // SDs of penalized spline coefficients
  // parameters for spline 5
  // standardized penalized spline coefficients
  vector[knots_5[1]] zs_5_1;
  vector<lower=0>[nb_5] sds_5;  // SDs of penalized spline coefficients
  real<lower=0> sigma;  // dispersion parameter
}
transformed parameters {
  // penalized spline coefficients
  vector[knots_1[1]] s_1_1;
  // penalized spline coefficients
  vector[knots_2[1]] s_2_1;
  // penalized spline coefficients
  vector[knots_3[1]] s_3_1;
  // penalized spline coefficients
  vector[knots_4[1]] s_4_1;
  // penalized spline coefficients
  vector[knots_5[1]] s_5_1;
  // prior contributions to the log posterior
  real lprior = 0;
  // compute penalized spline coefficients
  s_1_1 = sds_1[1] * zs_1_1;
  // compute penalized spline coefficients
  s_2_1 = sds_2[1] * zs_2_1;
  // compute penalized spline coefficients
  s_3_1 = sds_3[1] * zs_3_1;
  // compute penalized spline coefficients
  s_4_1 = sds_4[1] * zs_4_1;
  // compute penalized spline coefficients
  s_5_1 = sds_5[1] * zs_5_1;
  lprior += student_t_lpdf(Intercept | 3, -0.3, 2.5);
  lprior += student_t_lpdf(sds_1 | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += student_t_lpdf(sds_2 | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += student_t_lpdf(sds_3 | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += student_t_lpdf(sds_4 | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += student_t_lpdf(sds_5 | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
  lprior += student_t_lpdf(sigma | 3, 0, 2.5)
    - 1 * student_t_lccdf(0 | 3, 0, 2.5);
}
model {
  // likelihood including constants
  if (!prior_only) {
    // initialize linear predictor term
    vector[N] mu = rep_vector(0.0, N);
    mu += Intercept + Xs * bs + Zs_1_1 * s_1_1 + Zs_2_1 * s_2_1 + Zs_3_1 * s_3_1 + Zs_4_1 * s_4_1 + Zs_5_1 * s_5_1;
    target += normal_lpdf(Y | mu, sigma);
  }
  // priors including constants
  target += lprior;
  target += std_normal_lpdf(zs_1_1);
  target += std_normal_lpdf(zs_2_1);
  target += std_normal_lpdf(zs_3_1);
  target += std_normal_lpdf(zs_4_1);
  target += std_normal_lpdf(zs_5_1);
}
generated quantities {
  // actual population-level intercept
  real b_Intercept = Intercept;
}

