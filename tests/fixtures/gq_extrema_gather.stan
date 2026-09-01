// Phase 0 characterization: an indexed vector is an Eigen IndexedView. The
// current generated-quantities native extrema surface deliberately refuses
// it, so the write_array path must stay on WaInterp.
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
