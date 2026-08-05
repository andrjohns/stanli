// categorical_lpmf with scalar and array outcomes on a simplex parameter.
data {
  int<lower=1> K;
  int<lower=1, upper=K> y;
  array[3] int<lower=1, upper=K> ys;
}
parameters {
  simplex[K] theta;
}
model {
  target += categorical_lpmf(y | theta);
  target += categorical_lpmf(ys | theta);
}
