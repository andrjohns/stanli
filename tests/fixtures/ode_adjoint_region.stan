functions {
  vector rhs(real t, vector state, real rate, array[] real x_r,
             array[] int x_i) {
    return rep_vector(rate + 0 * t + 0 * sum(x_r) + 0 * sum(x_i), rows(state));
  }
}

transformed data {
  array[1] real x_r = {1.0};
  array[1] int x_i = {1};
  vector[1] atol = [1e-10]';
}

parameters {
  real gate;
  real initial;
  real start;
  real finish;
  real rate;
}

model {
  vector[1] y0 = [initial]';
  array[1] real ts = {finish};
  target += -0.5 * (square(gate) + square(initial) + square(start)
                    + square(finish) + square(rate));
  target += ode_adjoint_tol_ctl(rhs, y0, start, ts, 1e-10, atol, 1e-10,
                                atol, 1e-10, 1e-10, 100000, 10, 1, 1, 1,
                                rate, x_r, x_i)[1, 1];
  if (gate > 0) {
    target += ode_adjoint_tol_ctl(rhs, y0, start, ts, 1e-10, atol, 1e-10,
                                  atol, 1e-10, 1e-10, 100000, 10, 1, 1, 1,
                                  rate, x_r, x_i)[1, 1];
  }
}
