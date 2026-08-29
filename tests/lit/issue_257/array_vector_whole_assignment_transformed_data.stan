// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
// STANLI-LIT-DATA: {"T": 3}
data { int T; }
transformed data {
  vector[2] phi0 = [1, 2]';
  array[T] vector[2] mu;
  for (t in 1:T) mu[t, ] = phi0;
}
parameters { real y; }
model { y ~ normal(mu[1, 1], 1); }
