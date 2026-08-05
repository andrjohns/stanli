// Row of a 2-D int data array as a density's random variable
// (the Mb/Mt/irt_2pl pattern): y[i] must reach the kernel as a
// T-length int array, not a scalar.
data {
  int<lower=0> M;
  int<lower=0> T;
  array[M, T] int<lower=0, upper=1> y;
}
parameters {
  vector<lower=0, upper=1>[T] p;
}
model {
  for (i in 1 : M) {
    target += bernoulli_lpmf(y[i] | p);
  }
}
