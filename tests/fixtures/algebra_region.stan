functions {
  vector identity_system(vector x, vector theta, array[] real x_r,
                         array[] int x_i) {
    return x - theta;
  }
}

transformed data {
  array[0] real x_r;
  array[0] int x_i;
}

parameters {
  real gate;
  vector[1] theta;
}

model {
  target += -0.5 * (square(gate) + dot_self(theta));
  if (gate > 0) {
    vector[1] solved = algebra_solver(identity_system, rep_vector(0, 1),
                                      theta, x_r, x_i);
    vector[1] solved_newton = algebra_solver_newton(
        identity_system, rep_vector(0, 1), theta, x_r, x_i);
    vector[1] solved_variadic_newton =
        solve_newton(identity_system, rep_vector(0, 1), theta, x_r, x_i);
    vector[1] solved_variadic_powell =
        solve_powell(identity_system, rep_vector(0, 1), theta, x_r, x_i);
    target += solved[1] + solved_newton[1] + solved_variadic_newton[1]
              + solved_variadic_powell[1];
  }
}
