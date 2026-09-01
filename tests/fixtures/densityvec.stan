// unsupported_normal_lpdf_container, verbatim: a container-valued
// normal_lpdf call inside a data-dependent while loop, which has to compile
// to the register machine (a runtime-control region) rather than the graph.
// No parameters: lp is a fixed sum of N identical vectorized calls, which is
// exactly what pins the register machine's new container support against a
// hand-computed reference instead of masking a gradient bug.
functions {
  real container_normal(vector y, row_vector mu, row_vector sigma) {
    return normal_lpdf(y | mu, sigma);
  }
}
data {
  int<lower=1> N;
  vector[N] y;
  row_vector[N] mu;
  row_vector<lower=0>[N] sigma;
}
model {
  int i = 1;
  while (i <= N) {
    target += container_normal(y, mu, sigma);
    i += 1;
  }
}
