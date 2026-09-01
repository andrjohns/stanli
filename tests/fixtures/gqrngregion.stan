// RNG draws inside a runtime-control region. The while loop puts the whole
// generated-quantities section on the register machine, which reaches the
// graph's own OP_RNG kernel through Program::CALL rather than transcribing
// each family -- so the draws, and their positions in the stream, have to
// come out identical to the interpreter's.
data {
  int<lower=1> K;
}
parameters {
  real<lower=0> sigma;
}
model {
  sigma ~ lognormal(0, 1);
}
generated quantities {
  real e = 0;
  real n = 0;
  vector[K] d = rep_vector(0, K);
  vector[K] mn = rep_vector(0, K);
  int i = 1;
  while (i <= 1) {
    e = exponential_rng(sigma);
    n = normal_rng(0.5, sigma);
    d = dirichlet_rng(rep_vector(1.5, K));
    mn = multi_normal_rng(rep_vector(0.25, K),
                          diag_matrix(rep_vector(sigma, K)));
    i += 1;
  }
}
