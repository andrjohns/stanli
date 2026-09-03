functions {
  vector rhs(real t, vector state) {
    return rep_vector(1 + 0 * t, rows(state));
  }
}

parameters {
  real start_log;
  real finish_log;
}

model {
  target += -0.5 * (square(start_log) + square(finish_log));
  target += ode_rk45(rhs, [0.0]', -exp(start_log), {0.0})[1, 1];
  target += ode_rk45(rhs, [0.0]', 0.0, {exp(finish_log)})[1, 1];
}
