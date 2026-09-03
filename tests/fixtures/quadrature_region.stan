functions {
  real legacy_integrand(real x, real xc, array[] real theta,
                        array[] real x_r, array[] int x_i) {
    return theta[1] * x + 0 * sum(x_r) + 0 * sum(x_i);
  }

  real variadic_integrand(real x, real xc, real scale, array[] real x_r,
                          array[] int x_i) {
    return scale * x + 0 * sum(x_r) + 0 * sum(x_i);
  }
}

transformed data {
  array[1] real x_r = {2.0};
  array[1] int x_i = {3};
}

parameters {
  real gate;
  real bound;
  real scale;
}

model {
  target += integrate_1d(legacy_integrand, 0, bound, {scale}, x_r, x_i);
  if (gate > 0) {
    target += integrate_1d_double_exponential(
        variadic_integrand, 0, bound, scale, x_r, x_i);
    target += integrate_1d_double_exponential_tol(
        variadic_integrand, 0, bound, 1e-8, 1e-12, 15, scale, x_r, x_i);
    target += integrate_1d_gauss_kronrod(
        variadic_integrand, 0, bound, scale, x_r, x_i);
    target += integrate_1d_gauss_kronrod_tol(
        variadic_integrand, 0, bound, 1e-8, 1e-12, 15, scale, x_r, x_i);
  }
}
