// Transformed-data use of the function vocabulary the ODE-side interpreter
// always had: fmax, fmin, inv, inv_logit. One interpreter, one vocabulary.
//
// The densities are here for a sharper reason. The compiled register
// program (mir_prog.hpp) falls back to this interpreter when compilation
// fails, so the interpreter's vocabulary has to be a superset of the
// compiler's -- otherwise a compile failure turns a slow path into an
// error. These four were the compiler's and not the interpreter's.
data {
  real a;
  real b;
}
transformed data {
  real m = fmax(a, b) + fmin(a, b) + inv(b) + inv_logit(a);
  real d = inv_gamma_lpdf(1.5 | 2, 3) + weibull_lpdf(1.5 | 2, 3)
           + logistic_lpdf(0.25 | 0, 1) + double_exponential_lpdf(0.25 | 0, 1);
}
parameters {
  real mu;
}
model {
  target += normal_lpdf(mu | m + d, 1);
}
