functions {
  vector system(vector unknown, real wanted) {
    return [square(unknown[1]) - wanted]';
  }
}

parameters {
  real wanted;
}

model {
  target += -0.5 * square(wanted);
  target += solve_powell_tol(system, [1.0]', 1e-10, 1e-6, 1, wanted)[1];
}
