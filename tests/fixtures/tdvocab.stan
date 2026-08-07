// Transformed-data use of the function vocabulary the ODE-side interpreter
// always had: fmax, fmin, inv, inv_logit. One interpreter, one vocabulary.
data {
  real a;
  real b;
}
transformed data {
  real m = fmax(a, b) + fmin(a, b) + inv(b) + inv_logit(a);
}
parameters {
  real mu;
}
model {
  target += normal_lpdf(mu | m, 1);
}
