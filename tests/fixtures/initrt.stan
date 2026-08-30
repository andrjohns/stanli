data {
  int<lower=1> N;
  vector[N] lo;
}
parameters {
  real mu;
  real<lower=0> sigma;
  vector<lower=lo>[N] v;
  array[2] simplex[3] s;
  array[2] matrix[3, 2] m;
  cholesky_factor_corr[3] L;
  sum_to_zero_vector[4] z;
  array[2] ordered[3] o;
  real<lower=mu> dep;
}
model {
  mu ~ normal(0, 1);
  sigma ~ normal(0, 1);
  v ~ normal(0, 1);
  for (i in 1 : 2) s[i] ~ dirichlet(rep_vector(1.0, 3));
  for (i in 1 : 2) to_vector(m[i]) ~ normal(0, 1);
  L ~ lkj_corr_cholesky(2);
  z ~ normal(0, 1);
  for (i in 1 : 2) o[i] ~ normal(0, 1);
  dep ~ normal(0, 1);
}
