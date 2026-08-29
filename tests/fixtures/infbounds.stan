// Infinite declaration bounds. Every *_constrain overload reads an infinite
// bound as no bound at all: the element passes through untouched and adds
// no jacobian term. The kernels used to exponentiate through it, so a
// declaration carrying one produced inf and an lp of -inf.
//
// The bound vectors mix infinite and finite entries, which is what makes
// them worth testing -- the finite entries still have to transform. Their
// four elements cover every branch lower_upper resolves per element:
// lower-only infinite, upper-only infinite, both, and neither.
//
// Each parameter is summed into its own target term, so the reference in
// tests/test_lower.cpp can reassociate the same way.
transformed data {
  vector[4] lo = [negative_infinity(), -0.5, negative_infinity(), 0.75]';
  vector[4] hi = [3.25, positive_infinity(), positive_infinity(), 4.5]';
}
parameters {
  vector<lower = lo>[4] a;
  vector<upper = hi>[4] b;
  vector<lower = lo, upper = hi>[4] c;
  // The whole-container case: one shared bound that is itself infinite.
  real<lower = negative_infinity()> d;
  real<upper = positive_infinity()> e;
}
model {
  target += sum(a);
  target += sum(b);
  target += sum(c);
  target += d;
  target += e;
}
