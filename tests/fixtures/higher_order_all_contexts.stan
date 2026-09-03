// Exhaustive higher-order runtime-path coverage for the pinned stanc.
//
// The overload inventory was obtained with:
//   deps/stanc3/stanc --dump-stan-math-signatures
//
// That inventory contains 14 higher-order signatures: four legacy algebra
// solver overloads, two integrate_1d overloads, generic integrate_ode, six
// named legacy integrate_ode overloads, and map_rect.  Modern solvers and
// reduce_sum are typechecked as
// compiler-special variadic functions and are absent from that flat dump.
// This model adds all 21 supported compiler-special variants, for 35 cases in
// each of four execution contexts:
//
//   td       transformed-data interpretation
//   graph    parameter-dependent graph lowering
//   runtime  parameter-controlled runtime-program lowering
//   gq       interpreted generated quantities / write_array

functions {
  vector legacy_system(vector unknown, vector theta, array[] real x_r,
                       array[] int x_i) {
    return unknown - [theta[1]]';
  }

  vector variadic_system(vector unknown, real wanted, array[] real x_r,
                         array[] int x_i) {
    return unknown - [wanted]';
  }

  real legacy_integrand(real x, real xc, array[] real theta,
                        array[] real x_r, array[] int x_i) {
    return theta[1] * x;
  }

  real variadic_integrand(real x, real xc, real scale, array[] real x_r,
                          array[] int x_i) {
    return scale * x;
  }

  array[] real legacy_rhs(real t, array[] real state, array[] real theta,
                          array[] real x_r, array[] int x_i) {
    return {theta[1]};
  }

  vector rhs(real t, vector state, real rate, array[] real x_r,
             array[] int x_i) {
    return rep_vector(rate, rows(state));
  }

  vector residual(real t, vector state, vector derivative, real rate,
                  array[] real x_r, array[] int x_i) {
    return derivative - rep_vector(rate, rows(state));
  }

  vector mapped(vector shared, vector job, array[] real x_r,
                array[] int x_i) {
    return [shared[1] + job[1] + sum(x_r) + sum(x_i)]';
  }

  real partial_sum(array[] real slice, int start, int end, real scale,
                   array[] real x_r, array[] int x_i) {
    return scale * sum(slice);
  }
}

transformed data {
  array[1] real td_x_r = {0.25};
  array[1] int td_x_i = {1};
  array[1] real td_times = {0.1};
  array[1] real td_theta = {0.4};
  vector[1] td_y0 = [0.2]';
  vector[1] td_yp0 = [0.4]';
  vector[1] td_atol = [1e-8]';
  array[1] vector[1] td_jobs = {[0.3]'};
  array[1, 1] real td_job_x_r = {{0.25}};
  array[1, 1] int td_job_x_i = {{1}};
  array[2] real td_slice = {0.2, 0.3};
  real td_total = 0;

  // The 14 signatures present in --dump-stan-math-signatures.
  td_total += sum(algebra_solver(legacy_system, [0.0]', [0.4]', td_x_r,
                                 td_x_i));
  td_total += sum(algebra_solver(legacy_system, [0.0]', [0.4]', td_x_r,
                                 td_x_i, 1e-8, 1e-8, 100));
  td_total += sum(algebra_solver_newton(legacy_system, [0.0]', [0.4]',
                                        td_x_r, td_x_i));
  td_total += sum(algebra_solver_newton(legacy_system, [0.0]', [0.4]',
                                        td_x_r, td_x_i, 1e-8, 1e-8, 100));
  td_total += integrate_1d(legacy_integrand, 0, 0.1, td_theta, td_x_r,
                           td_x_i);
  td_total += integrate_1d(legacy_integrand, 0, 0.1, td_theta, td_x_r,
                           td_x_i, 1e-8);
  td_total += integrate_ode(legacy_rhs, {0.2}, 0, td_times, td_theta,
                            td_x_r, td_x_i)[1][1];
  td_total += integrate_ode_rk45(legacy_rhs, {0.2}, 0, td_times, td_theta,
                                 td_x_r, td_x_i)[1][1];
  td_total += integrate_ode_rk45(legacy_rhs, {0.2}, 0, td_times, td_theta,
                                 td_x_r, td_x_i, 1e-8, 1e-8, 1000)[1][1];
  td_total += integrate_ode_bdf(legacy_rhs, {0.2}, 0, td_times, td_theta,
                                td_x_r, td_x_i)[1][1];
  td_total += integrate_ode_bdf(legacy_rhs, {0.2}, 0, td_times, td_theta,
                                td_x_r, td_x_i, 1e-8, 1e-8, 1000)[1][1];
  td_total += integrate_ode_adams(legacy_rhs, {0.2}, 0, td_times, td_theta,
                                  td_x_r, td_x_i)[1][1];
  td_total += integrate_ode_adams(legacy_rhs, {0.2}, 0, td_times, td_theta,
                                  td_x_r, td_x_i, 1e-8, 1e-8, 1000)[1][1];
  td_total += sum(map_rect(mapped, [0.4]', td_jobs, td_job_x_r,
                           td_job_x_i));

  // The 21 compiler-special higher-order variants absent from the flat dump.
  td_total += reduce_sum(partial_sum, td_slice, 1, 0.4, td_x_r, td_x_i);
  td_total += reduce_sum_static(partial_sum, td_slice, 1, 0.4, td_x_r,
                                td_x_i);
  td_total += sum(solve_newton(variadic_system, [0.0]', 0.4, td_x_r,
                               td_x_i));
  td_total += sum(solve_newton_tol(variadic_system, [0.0]', 1e-8, 1e-8,
                                   100, 0.4, td_x_r, td_x_i));
  td_total += sum(solve_powell(variadic_system, [0.0]', 0.4, td_x_r,
                               td_x_i));
  td_total += sum(solve_powell_tol(variadic_system, [0.0]', 1e-8, 1e-8,
                                   100, 0.4, td_x_r, td_x_i));
  td_total += integrate_1d_double_exponential(
      variadic_integrand, 0, 0.1, 0.4, td_x_r, td_x_i);
  td_total += integrate_1d_double_exponential_tol(
      variadic_integrand, 0, 0.1, 1e-8, 1e-12, 15, 0.4, td_x_r, td_x_i);
  td_total += integrate_1d_gauss_kronrod(
      variadic_integrand, 0, 0.1, 0.4, td_x_r, td_x_i);
  td_total += integrate_1d_gauss_kronrod_tol(
      variadic_integrand, 0, 0.1, 1e-8, 1e-12, 15, 0.4, td_x_r, td_x_i);
  td_total += ode_rk45(rhs, td_y0, 0, td_times, 0.4, td_x_r,
                       td_x_i)[1][1];
  td_total += ode_rk45_tol(rhs, td_y0, 0, td_times, 1e-8, 1e-8, 1000,
                           0.4, td_x_r, td_x_i)[1][1];
  td_total += ode_bdf(rhs, td_y0, 0, td_times, 0.4, td_x_r,
                      td_x_i)[1][1];
  td_total += ode_bdf_tol(rhs, td_y0, 0, td_times, 1e-8, 1e-8, 1000,
                          0.4, td_x_r, td_x_i)[1][1];
  td_total += ode_adams(rhs, td_y0, 0, td_times, 0.4, td_x_r,
                        td_x_i)[1][1];
  td_total += ode_adams_tol(rhs, td_y0, 0, td_times, 1e-8, 1e-8, 1000,
                            0.4, td_x_r, td_x_i)[1][1];
  td_total += ode_ckrk(rhs, td_y0, 0, td_times, 0.4, td_x_r,
                       td_x_i)[1][1];
  td_total += ode_ckrk_tol(rhs, td_y0, 0, td_times, 1e-8, 1e-8, 1000,
                           0.4, td_x_r, td_x_i)[1][1];
  td_total += dae(residual, td_y0, td_yp0, 0, td_times, 0.4, td_x_r,
                  td_x_i)[1][1];
  td_total += dae_tol(residual, td_y0, td_yp0, 0, td_times, 1e-8, 1e-8,
                      1000, 0.4, td_x_r, td_x_i)[1][1];
  td_total += ode_adjoint_tol_ctl(
      rhs, td_y0, 0, td_times, 1e-8, td_atol, 1e-8, td_atol, 1e-8, 1e-8,
      1000, 10, 1, 1, 1, 0.4, td_x_r, td_x_i)[1][1];
}

parameters {
  real gate;
  real initial;
  real derivative;
  real start;
  real log_duration;
  real rate;
  real wanted;
}

transformed parameters {
  real graph_total = 0;

  graph_total += sum(algebra_solver(legacy_system, [0.0]', [wanted]',
                                    td_x_r, td_x_i));
  graph_total += sum(algebra_solver(legacy_system, [0.0]', [wanted]',
                                    td_x_r, td_x_i, 1e-8, 1e-8, 100));
  graph_total += sum(algebra_solver_newton(legacy_system, [0.0]', [wanted]',
                                           td_x_r, td_x_i));
  graph_total += sum(algebra_solver_newton(
      legacy_system, [0.0]', [wanted]', td_x_r, td_x_i, 1e-8, 1e-8, 100));
  graph_total += integrate_1d(legacy_integrand, 0, 0.1, {rate},
                              td_x_r, td_x_i);
  graph_total += integrate_1d(legacy_integrand, 0, 0.1, {rate},
                              td_x_r, td_x_i, 1e-8);
  graph_total += integrate_ode(
      legacy_rhs, {initial}, 0, td_times, {rate}, td_x_r, td_x_i)[1][1];
  graph_total += integrate_ode_rk45(
      legacy_rhs, {initial}, 0, td_times, {rate}, td_x_r, td_x_i)[1][1];
  graph_total += integrate_ode_rk45(
      legacy_rhs, {initial}, 0, td_times, {rate}, td_x_r, td_x_i,
      1e-8, 1e-8, 1000)[1][1];
  graph_total += integrate_ode_bdf(
      legacy_rhs, {initial}, 0, td_times, {rate}, td_x_r, td_x_i)[1][1];
  graph_total += integrate_ode_bdf(
      legacy_rhs, {initial}, 0, td_times, {rate}, td_x_r, td_x_i,
      1e-8, 1e-8, 1000)[1][1];
  graph_total += integrate_ode_adams(
      legacy_rhs, {initial}, 0, td_times, {rate}, td_x_r, td_x_i)[1][1];
  graph_total += integrate_ode_adams(
      legacy_rhs, {initial}, 0, td_times, {rate}, td_x_r, td_x_i,
      1e-8, 1e-8, 1000)[1][1];
  graph_total += sum(map_rect(mapped, [rate]', {[0.3 * rate]'}, td_job_x_r,
                              td_job_x_i));

  graph_total += reduce_sum(partial_sum, {initial, rate}, 1, rate, td_x_r,
                            td_x_i);
  graph_total += reduce_sum_static(partial_sum, {initial, rate}, 1, rate,
                                   td_x_r, td_x_i);
  graph_total += sum(solve_newton(variadic_system, [0.0]', wanted, td_x_r,
                                  td_x_i));
  graph_total += sum(solve_newton_tol(variadic_system, [0.0]', 1e-8, 1e-8,
                                      100, wanted, td_x_r, td_x_i));
  graph_total += sum(solve_powell(variadic_system, [0.0]', wanted, td_x_r,
                                  td_x_i));
  graph_total += sum(solve_powell_tol(variadic_system, [0.0]', 1e-8, 1e-8,
                                      100, wanted, td_x_r, td_x_i));
  graph_total += integrate_1d_double_exponential(
      variadic_integrand, 0, 0.1, rate, td_x_r, td_x_i);
  graph_total += integrate_1d_double_exponential_tol(
      variadic_integrand, 0, 0.1, 1e-8, 1e-12, 15, rate, td_x_r, td_x_i);
  graph_total += integrate_1d_gauss_kronrod(
      variadic_integrand, 0, 0.1, rate, td_x_r, td_x_i);
  graph_total += integrate_1d_gauss_kronrod_tol(
      variadic_integrand, 0, 0.1, 1e-8, 1e-12, 15, rate, td_x_r, td_x_i);
  graph_total += ode_rk45(rhs, [initial]', start,
                          {start + exp(log_duration)}, rate, td_x_r,
                          td_x_i)[1][1];
  graph_total += ode_rk45_tol(rhs, [initial]', start,
                              {start + exp(log_duration)}, 1e-8, 1e-8,
                              1000, rate, td_x_r, td_x_i)[1][1];
  graph_total += ode_bdf(rhs, [initial]', start,
                         {start + exp(log_duration)}, rate, td_x_r,
                         td_x_i)[1][1];
  graph_total += ode_bdf_tol(rhs, [initial]', start,
                             {start + exp(log_duration)}, 1e-8, 1e-8,
                             1000, rate, td_x_r, td_x_i)[1][1];
  graph_total += ode_adams(rhs, [initial]', start,
                           {start + exp(log_duration)}, rate, td_x_r,
                           td_x_i)[1][1];
  graph_total += ode_adams_tol(rhs, [initial]', start,
                               {start + exp(log_duration)}, 1e-8, 1e-8,
                               1000, rate, td_x_r, td_x_i)[1][1];
  graph_total += ode_ckrk(rhs, [initial]', start,
                          {start + exp(log_duration)}, rate, td_x_r,
                          td_x_i)[1][1];
  graph_total += ode_ckrk_tol(rhs, [initial]', start,
                              {start + exp(log_duration)}, 1e-8, 1e-8,
                              1000, rate, td_x_r, td_x_i)[1][1];
  graph_total += dae(residual, [initial]', [derivative]', 0, td_times, rate,
                     td_x_r, td_x_i)[1][1];
  graph_total += dae_tol(residual, [initial]', [derivative]', 0, td_times,
                         1e-8, 1e-8, 1000, rate, td_x_r, td_x_i)[1][1];
  graph_total += ode_adjoint_tol_ctl(
      rhs, [initial]', start, {start + exp(log_duration)}, 1e-8, td_atol,
      1e-8, td_atol, 1e-8, 1e-8, 1000, 10, 1, 1, 1, rate, td_x_r,
      td_x_i)[1][1];
}

model {
  target += normal_lpdf(gate | 0, 1) + normal_lpdf(initial | 0.2, 1)
            + normal_lpdf(derivative | rate, 1) + normal_lpdf(start | 0, 1)
            + normal_lpdf(log_duration | -2, 1) + normal_lpdf(rate | 0.4, 1)
            + normal_lpdf(wanted | 0.4, 1) + td_total;
  target += graph_total;

  if (gate > 0) {
    target += sum(algebra_solver(legacy_system, [0.0]', [wanted]', td_x_r,
                                 td_x_i));
    target += sum(algebra_solver(legacy_system, [0.0]', [wanted]', td_x_r,
                                 td_x_i, 1e-8, 1e-8, 100));
    target += sum(algebra_solver_newton(legacy_system, [0.0]', [wanted]',
                                        td_x_r, td_x_i));
    target += sum(algebra_solver_newton(legacy_system, [0.0]', [wanted]',
                                        td_x_r, td_x_i, 1e-8, 1e-8, 100));
    target += integrate_1d(legacy_integrand, 0, 0.1, {rate}, td_x_r,
                           td_x_i);
    target += integrate_1d(legacy_integrand, 0, 0.1, {rate}, td_x_r,
                           td_x_i, 1e-8);
    target += integrate_ode(legacy_rhs, {initial}, 0, td_times, {rate},
                            td_x_r, td_x_i)[1][1];
    target += integrate_ode_rk45(legacy_rhs, {initial}, 0, td_times,
                                 {rate}, td_x_r, td_x_i)[1][1];
    target += integrate_ode_rk45(legacy_rhs, {initial}, 0, td_times,
                                 {rate}, td_x_r, td_x_i, 1e-8, 1e-8,
                                 1000)[1][1];
    target += integrate_ode_bdf(legacy_rhs, {initial}, 0, td_times,
                                {rate}, td_x_r, td_x_i)[1][1];
    target += integrate_ode_bdf(legacy_rhs, {initial}, 0, td_times,
                                {rate}, td_x_r, td_x_i, 1e-8, 1e-8,
                                1000)[1][1];
    target += integrate_ode_adams(legacy_rhs, {initial}, 0, td_times,
                                  {rate}, td_x_r, td_x_i)[1][1];
    target += integrate_ode_adams(legacy_rhs, {initial}, 0, td_times,
                                  {rate}, td_x_r, td_x_i, 1e-8, 1e-8,
                                  1000)[1][1];
    target += sum(map_rect(mapped, [rate]', {[0.3 * rate]'}, td_job_x_r,
                           td_job_x_i));

    target += reduce_sum(partial_sum, {initial, rate}, 1, rate, td_x_r, td_x_i);
    target += reduce_sum_static(partial_sum, {initial, rate}, 1, rate, td_x_r,
                                td_x_i);
    target += sum(solve_newton(variadic_system, [0.0]', wanted, td_x_r,
                               td_x_i));
    target += sum(solve_newton_tol(variadic_system, [0.0]', 1e-8, 1e-8,
                                   100, wanted, td_x_r, td_x_i));
    target += sum(solve_powell(variadic_system, [0.0]', wanted, td_x_r,
                               td_x_i));
    target += sum(solve_powell_tol(variadic_system, [0.0]', 1e-8, 1e-8,
                                   100, wanted, td_x_r, td_x_i));
    target += integrate_1d_double_exponential(
        variadic_integrand, 0, 0.1, rate, td_x_r, td_x_i);
    target += integrate_1d_double_exponential_tol(
        variadic_integrand, 0, 0.1, 1e-8, 1e-12, 15, rate, td_x_r, td_x_i);
    target += integrate_1d_gauss_kronrod(
        variadic_integrand, 0, 0.1, rate, td_x_r, td_x_i);
    target += integrate_1d_gauss_kronrod_tol(
        variadic_integrand, 0, 0.1, 1e-8, 1e-12, 15, rate, td_x_r, td_x_i);
    target += ode_rk45(rhs, [initial]', start, {start + exp(log_duration)}, rate, td_x_r,
                       td_x_i)[1][1];
    target += ode_rk45_tol(rhs, [initial]', start, {start + exp(log_duration)}, 1e-8, 1e-8,
                           1000, rate, td_x_r, td_x_i)[1][1];
    target += ode_bdf(rhs, [initial]', start, {start + exp(log_duration)}, rate, td_x_r,
                      td_x_i)[1][1];
    target += ode_bdf_tol(rhs, [initial]', start, {start + exp(log_duration)}, 1e-8, 1e-8,
                          1000, rate, td_x_r, td_x_i)[1][1];
    target += ode_adams(rhs, [initial]', start, {start + exp(log_duration)}, rate, td_x_r,
                        td_x_i)[1][1];
    target += ode_adams_tol(rhs, [initial]', start, {start + exp(log_duration)}, 1e-8, 1e-8,
                            1000, rate, td_x_r, td_x_i)[1][1];
    target += ode_ckrk(rhs, [initial]', start, {start + exp(log_duration)}, rate, td_x_r,
                       td_x_i)[1][1];
    target += ode_ckrk_tol(rhs, [initial]', start, {start + exp(log_duration)}, 1e-8, 1e-8,
                           1000, rate, td_x_r, td_x_i)[1][1];
    target += dae(residual, [initial]', [derivative]', 0, td_times, rate,
                  td_x_r, td_x_i)[1][1];
    target += dae_tol(residual, [initial]', [derivative]', 0, td_times, 1e-8,
                      1e-8, 1000, rate, td_x_r, td_x_i)[1][1];
    target += ode_adjoint_tol_ctl(
        rhs, [initial]', start, {start + exp(log_duration)}, 1e-8, td_atol, 1e-8,
        td_atol, 1e-8, 1e-8, 1000, 10, 1, 1, 1, rate, td_x_r,
        td_x_i)[1][1];
  }
}

generated quantities {
  real gq_total = 0;
  real td_total_out = td_total;

  gq_total += sum(algebra_solver(legacy_system, [0.0]', [wanted]', td_x_r,
                                 td_x_i));
  gq_total += sum(algebra_solver(legacy_system, [0.0]', [wanted]', td_x_r,
                                 td_x_i, 1e-8, 1e-8, 100));
  gq_total += sum(algebra_solver_newton(legacy_system, [0.0]', [wanted]',
                                        td_x_r, td_x_i));
  gq_total += sum(algebra_solver_newton(legacy_system, [0.0]', [wanted]',
                                        td_x_r, td_x_i, 1e-8, 1e-8, 100));
  gq_total += integrate_1d(legacy_integrand, 0, 0.1, {rate}, td_x_r,
                           td_x_i);
  gq_total += integrate_1d(legacy_integrand, 0, 0.1, {rate}, td_x_r,
                           td_x_i, 1e-8);
  gq_total += integrate_ode(legacy_rhs, {initial}, 0, td_times, {rate},
                            td_x_r, td_x_i)[1][1];
  gq_total += integrate_ode_rk45(legacy_rhs, {initial}, 0, td_times,
                                 {rate}, td_x_r, td_x_i)[1][1];
  gq_total += integrate_ode_rk45(legacy_rhs, {initial}, 0, td_times,
                                 {rate}, td_x_r, td_x_i, 1e-8, 1e-8,
                                 1000)[1][1];
  gq_total += integrate_ode_bdf(legacy_rhs, {initial}, 0, td_times,
                                {rate}, td_x_r, td_x_i)[1][1];
  gq_total += integrate_ode_bdf(legacy_rhs, {initial}, 0, td_times,
                                {rate}, td_x_r, td_x_i, 1e-8, 1e-8,
                                1000)[1][1];
  gq_total += integrate_ode_adams(legacy_rhs, {initial}, 0, td_times,
                                  {rate}, td_x_r, td_x_i)[1][1];
  gq_total += integrate_ode_adams(legacy_rhs, {initial}, 0, td_times,
                                  {rate}, td_x_r, td_x_i, 1e-8, 1e-8,
                                  1000)[1][1];
  gq_total += sum(map_rect(mapped, [rate]', {[0.3 * rate]'}, td_job_x_r,
                           td_job_x_i));

  gq_total += reduce_sum(partial_sum, {initial, rate}, 1, rate, td_x_r, td_x_i);
  gq_total += reduce_sum_static(partial_sum, {initial, rate}, 1, rate, td_x_r,
                                td_x_i);
  gq_total += sum(solve_newton(variadic_system, [0.0]', wanted, td_x_r,
                               td_x_i));
  gq_total += sum(solve_newton_tol(variadic_system, [0.0]', 1e-8, 1e-8,
                                   100, wanted, td_x_r, td_x_i));
  gq_total += sum(solve_powell(variadic_system, [0.0]', wanted, td_x_r,
                               td_x_i));
  gq_total += sum(solve_powell_tol(variadic_system, [0.0]', 1e-8, 1e-8,
                                   100, wanted, td_x_r, td_x_i));
  gq_total += integrate_1d_double_exponential(
      variadic_integrand, 0, 0.1, rate, td_x_r, td_x_i);
  gq_total += integrate_1d_double_exponential_tol(
      variadic_integrand, 0, 0.1, 1e-8, 1e-12, 15, rate, td_x_r, td_x_i);
  gq_total += integrate_1d_gauss_kronrod(
      variadic_integrand, 0, 0.1, rate, td_x_r, td_x_i);
  gq_total += integrate_1d_gauss_kronrod_tol(
      variadic_integrand, 0, 0.1, 1e-8, 1e-12, 15, rate, td_x_r, td_x_i);
  gq_total += ode_rk45(rhs, [initial]', start, {start + exp(log_duration)}, rate, td_x_r,
                       td_x_i)[1][1];
  gq_total += ode_rk45_tol(rhs, [initial]', start, {start + exp(log_duration)}, 1e-8, 1e-8,
                           1000, rate, td_x_r, td_x_i)[1][1];
  gq_total += ode_bdf(rhs, [initial]', start, {start + exp(log_duration)}, rate, td_x_r,
                      td_x_i)[1][1];
  gq_total += ode_bdf_tol(rhs, [initial]', start, {start + exp(log_duration)}, 1e-8, 1e-8,
                          1000, rate, td_x_r, td_x_i)[1][1];
  gq_total += ode_adams(rhs, [initial]', start, {start + exp(log_duration)}, rate, td_x_r,
                        td_x_i)[1][1];
  gq_total += ode_adams_tol(rhs, [initial]', start, {start + exp(log_duration)}, 1e-8, 1e-8,
                            1000, rate, td_x_r, td_x_i)[1][1];
  gq_total += ode_ckrk(rhs, [initial]', start, {start + exp(log_duration)}, rate, td_x_r,
                       td_x_i)[1][1];
  gq_total += ode_ckrk_tol(rhs, [initial]', start, {start + exp(log_duration)}, 1e-8, 1e-8,
                           1000, rate, td_x_r, td_x_i)[1][1];
  gq_total += dae(residual, [initial]', [derivative]', 0, td_times, rate,
                  td_x_r, td_x_i)[1][1];
  gq_total += dae_tol(residual, [initial]', [derivative]', 0, td_times, 1e-8,
                      1e-8, 1000, rate, td_x_r, td_x_i)[1][1];
  gq_total += ode_adjoint_tol_ctl(
      rhs, [initial]', start, {start + exp(log_duration)}, 1e-8, td_atol, 1e-8, td_atol,
      1e-8, 1e-8, 1000, 10, 1, 1, 1, rate, td_x_r, td_x_i)[1][1];
}
