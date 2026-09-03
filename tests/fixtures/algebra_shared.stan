functions {
  vector system(vector unknown, real wanted) {
    return unknown - [wanted]';
  }
}

parameters {
  real gate;
  real wanted;
}

model {
  target += -0.5 * (square(gate) + square(wanted));
  vector[1] a = solve_newton(system, [0.0]', wanted);
  vector[1] b = solve_powell(system, [0.0]', wanted);
  vector[1] at = solve_newton_tol(system, [0.0]', 1e-8, 1e-8, 100, wanted);
  vector[1] bt = solve_powell_tol(system, [0.0]', 1e-8, 1e-8, 100, wanted);
  target += a[1] + b[1] + at[1] + bt[1];
  if (gate > 0) {
    target += solve_newton(system, [0.0]', wanted)[1];
    target += solve_powell(system, [0.0]', wanted)[1];
    target += solve_newton_tol(system, [0.0]', 1e-8, 1e-8, 100, wanted)[1];
    target += solve_powell_tol(system, [0.0]', 1e-8, 1e-8, 100, wanted)[1];
  }
}
