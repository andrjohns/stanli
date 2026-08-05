data {
  int<lower=1> K;
}
parameters {
  simplex[K] theta;
}
model {
  theta ~ dirichlet(rep_vector(2.0, K));
}
