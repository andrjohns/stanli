#include portable_include_functions.stan
parameters {
  real x;
}
model {
  target += portable_helper_lpdf(x);
}
