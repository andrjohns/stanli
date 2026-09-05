// The LKJ densities in generated quantities, where the per-draw interpreter
// has to answer them whenever anything else in the section makes the graph
// give the whole thing up.
data {
  int K;
}
parameters {
  cholesky_factor_corr[K] L;
  real<lower=0> eta;
}
model {
  L ~ lkj_corr_cholesky(eta);
  eta ~ gamma(2, 2);
}
generated quantities {
  corr_matrix[K] R = multiply_lower_tri_self_transpose(L);
  real lchol = lkj_corr_cholesky_lpdf(L | eta);
  real lcorr = lkj_corr_lpdf(R | eta);
}
