// Control flow that depends on a parameter: neither the if nor the
// ternary can pick an arm when the model is loaded, so each becomes a
// necessity island (lower.cpp) rather than a compile error. Both arms of
// both are reachable across the points tests/test_lower.cpp evaluates.
//
// The if assigns TWO variables, so its island has two live-outs and the
// extractions after it have to take their own offsets.
data {
  real y;
}
parameters {
  real mu;
  real<lower=0> sigma;
}
model {
  real m;
  real w;
  if (mu > 0) {
    m = mu * 2;
    w = 1 + sigma;
  } else {
    m = -mu;
    w = 3 * sigma;
  }
  real s = sigma < 1 ? 1 / sigma : sigma;
  y ~ normal(m, s * w);
}
