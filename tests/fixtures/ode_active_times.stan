functions {
  vector rhs(real t, vector state, real rate) {
    return rep_vector(rate + 0 * t, rows(state));
  }
}

parameters {
  real gate;
  real initial;
  real start;
  real duration;
  real rate;
}

model {
  vector[1] y0 = [initial]';
  array[1] real ts = {start + exp(duration)};
  target += -0.5 * (square(gate) + square(initial) + square(start)
                    + square(duration) + square(rate));
  target += ode_rk45(rhs, y0, start, ts, rate)[1, 1];
  if (gate > 0)
    target += ode_rk45_tol(rhs, y0, start, ts, 1e-10, 1e-10, 100000,
                           rate)[1, 1];
}
