// brms writes its category-specific ordinal models (sratio, cratio and acat
// with cs()) as an lpmf whose body opens `int nthres = num_elements(thres);`
// and then walks `while (k <= min(y, nthres))`, and passes
// `Intercept - transpose(mucs[n])` for `thres`. The while can only unroll
// where that shape query is answered from the expression itself.
functions {
  real thres_lpdf(vector thres, int y) {
    int n = num_elements(thres);
    vector[n + 1] p;
    vector[n] q;
    int k = 1;
    while (k <= min(y, n)) {
      q[k] = log1m_inv_logit(thres[k]);
      p[k] = log1m_exp(q[k]);
      for (kk in 1 : (k - 1)) p[k] = p[k] + q[kk];
      k += 1;
    }
    if (y == n + 1) {
      p[n + 1] = sum(q);
    }
    return p[y];
  }
}
data {
  int<lower=1> N;
  int<lower=1> M;
  matrix[N, M] Xcs;
  array[N] int<lower=1> y;
}
parameters {
  vector[M] Intercept;
  matrix[M, M] bcs;
}
model {
  matrix[N, M] mucs = Xcs * bcs;
  Intercept ~ std_normal();
  to_vector(bcs) ~ std_normal();
  for (n in 1 : N) {
    target += thres_lpdf(Intercept - transpose(mucs[n]) | y[n]);
    target += thres_lpdf(transpose(mucs[n]) | y[n]);
  }
}
