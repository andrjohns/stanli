functions {
  vector rhs(real t, vector state, real rate) {
    return rep_vector(0 * rate, rows(state));
  }
  array[] real legacy_rhs(real t, array[] real state, array[] real theta,
                          array[] real x_r, array[] int x_i) {
    return {0 * theta[1] + 0 * sum(x_r) + 0 * sum(x_i)};
  }
}

transformed data {
  array[0] real x_r;
  array[0] int x_i;
  array[1] real ts = {0.1};
}

parameters {
  real gate;
  real initial;
  real rate;
}

model {
  target += -0.5 * (square(gate) + square(initial) + square(rate));
  if (gate > 0) {
    vector[1] y0 = [initial]';
    target += integrate_ode_rk45(legacy_rhs, {initial}, 0, ts, {rate}, x_r, x_i)[1, 1];
    target += integrate_ode_bdf(legacy_rhs, {initial}, 0, ts, {rate}, x_r, x_i)[1, 1];
    target += integrate_ode_adams(legacy_rhs, {initial}, 0, ts, {rate}, x_r, x_i)[1, 1];
    target += ode_rk45(rhs, y0, 0, ts, rate)[1, 1];
    target += ode_bdf(rhs, y0, 0, ts, rate)[1, 1];
    target += ode_adams(rhs, y0, 0, ts, rate)[1, 1];
    target += ode_ckrk(rhs, y0, 0, ts, rate)[1, 1];
    target += ode_rk45_tol(rhs, y0, 0, ts, 1e-8, 1e-8, 1000, rate)[1, 1];
    target += ode_bdf_tol(rhs, y0, 0, ts, 1e-8, 1e-8, 1000, rate)[1, 1];
    target += ode_adams_tol(rhs, y0, 0, ts, 1e-8, 1e-8, 1000, rate)[1, 1];
    target += ode_ckrk_tol(rhs, y0, 0, ts, 1e-8, 1e-8, 1000, rate)[1, 1];
  }
}
