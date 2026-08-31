// A profile block wraps ordinary statements purely for stanc's own timing
// output; stanli has no such output and should compile straight through as
// if the wrapper were not there.
data {
  real y;
}
parameters {
  real x;
}
model {
  profile("priors") {
    target += normal_lpdf(x | 0, 1);
  }
  profile("likelihood") {
    target += normal_lpdf(y | x, 1);
  }
}
