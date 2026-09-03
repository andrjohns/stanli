functions {
  real partial(array[] real slice, int start, int end) {
    return sum(slice) + 0 * start + 0 * end;
  }
  vector mapped(vector shared, vector job, array[] real x_r,
                array[] int x_i) {
    return [shared[1] + job[1] + sum(x_r) + sum(x_i)]';
  }
  vector system(vector unknown, real wanted) {
    return unknown - [wanted]';
  }
  real integrand(real x, real xc) {
    return x;
  }
  vector rhs(real t, vector state) {
    return rep_vector(1 + 0 * t, rows(state));
  }
  vector residual(real t, vector state, vector derivative) {
    return derivative - rep_vector(1 + 0 * t, rows(state));
  }
}

transformed data {
  real reduced = reduce_sum(partial, {1.0, 2.0}, 1);
  vector[1] mapped_value = map_rect(mapped, [1.0]', {[2.0]'}, {{3.0}}, {{4}});
  vector[1] root = solve_newton(system, [0.0]', 2.0);
  real area = integrate_1d_gauss_kronrod(integrand, 0.0, 1.0);
  array[1] vector[1] ode_value = ode_rk45(rhs, [0.0]', 0.0, {0.2});
  array[1] vector[1] dae_value = dae(residual, [0.0]', [1.0]', 0.0, {0.2});
  vector[1] atol = [1e-10]';
  array[1] vector[1] adjoint_value = ode_adjoint_tol_ctl(
      rhs, [0.0]', 0.0, {0.2}, 1e-10, atol, 1e-10, atol, 1e-10, 1e-10,
      100000, 10, 1, 1, 1);
  real expected = reduced + mapped_value[1] + root[1] + area
                  + ode_value[1, 1] + dae_value[1, 1]
                  + adjoint_value[1, 1];
}

parameters {
  real x;
}

model {
  x ~ normal(expected, 1);
}

generated quantities {
  vector[1] mapped_gq = map_rect(mapped, [1.0]', {[2.0]'}, {{3.0}}, {{4}});
  vector[1] root_gq = solve_powell_tol(system, [0.0]', 1e-8, 1e-8, 100,
                                       2.0);
  real area_gq = integrate_1d_double_exponential(integrand, 0.0, 1.0);
  array[1] vector[1] ode_gq = ode_bdf(rhs, [0.0]', 0.0, {0.2});
  array[1] vector[1] dae_gq = dae_tol(residual, [0.0]', [1.0]', 0.0, {0.2},
                                      1e-10, 1e-10, 100000);
  vector[1] atol_gq = [1e-10]';
  array[1] vector[1] adjoint_gq = ode_adjoint_tol_ctl(
      rhs, [0.0]', 0.0, {0.2}, 1e-10, atol_gq, 1e-10, atol_gq, 1e-10,
      1e-10, 100000, 10, 1, 1, 1);
}
