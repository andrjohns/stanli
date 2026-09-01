// An indexed vector is an Eigen IndexedView. The native path materializes the
// gather and retains packet grouping for the generated-quantities reduction.
data {
  int<lower=1> N;
  array[N] int<lower=1, upper=N> idx;
}
parameters {
  vector[N] x;
}
model {
  x ~ normal(0, 1);
}
generated quantities {
  real gathered_min = min(x[idx]);
}
