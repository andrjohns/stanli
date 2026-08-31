functions {
  vector rhs(real t, vector y, real rate) { return -rate * y; }
  array[] real legacy_rhs(real t, array[] real y, array[] real theta,
                           array[] real xr, array[] int xi) {
    return {-theta[1] * y[1]};
  }
  vector system(vector y, vector theta, array[] real xr, array[] int xi) {
    return y - theta;
  }
  real partial_sum(array[] real y, int start, int end, real rate) {
    return sum(y) * rate;
  }
  vector no_jobs(vector shared, vector job, data array[] real xr,
                   data array[] int xi) { return rep_vector(0, 1); }
}
transformed data {
  array[0] real xr;
  array[0] int xi;
  array[0] vector[0] jobs;
  array[0,0] real job_xr;
  array[0,0] int job_xi;
}
parameters { vector[2] theta; }
transformed parameters {
  real rate = 0.5 + 0.0625 * theta[1];
  vector[1] y0 = [1.0 + 0.0625 * theta[2]]';
  array[1] vector[1] rk = ode_rk45(rhs, y0, 0.0, {0.2}, rate);
  array[1] vector[1] bdf = ode_bdf(rhs, y0, 0.0, {0.2}, rate);
  array[1] vector[1] adams = ode_adams(rhs, y0, 0.0, {0.2}, rate);
  array[1] vector[1] ck = ode_ckrk(rhs, y0, 0.0, {0.2}, rate);
  array[1] vector[1] rkt = ode_rk45_tol(rhs, y0, 0.0, {0.2}, 1e-8, 1e-8, 10000, rate);
  array[1] vector[1] bdft = ode_bdf_tol(rhs, y0, 0.0, {0.2}, 1e-8, 1e-8, 10000, rate);
  array[1] vector[1] adamst = ode_adams_tol(rhs, y0, 0.0, {0.2}, 1e-8, 1e-8, 10000, rate);
  array[1] vector[1] ckt = ode_ckrk_tol(rhs, y0, 0.0, {0.2}, 1e-8, 1e-8, 10000, rate);
  array[1,1] real lrk = integrate_ode_rk45(legacy_rhs, {y0[1]}, 0.0, {0.2}, {rate}, xr, xi);
  array[1,1] real lbdf = integrate_ode_bdf(legacy_rhs, {y0[1]}, 0.0, {0.2}, {rate}, xr, xi);
  array[1,1] real ladams = integrate_ode_adams(legacy_rhs, {y0[1]}, 0.0, {0.2}, {rate}, xr, xi);
  vector[2] root = algebra_solver(system, [0.1, 0.2]', theta, xr, xi);
  real reduced = reduce_sum(partial_sum, {y0[1], 2.0}, 1, rate);
  real static_reduced = reduce_sum_static(partial_sum, {y0[1], 2.0}, 1, rate);
  vector[0] mapped = map_rect(no_jobs, rep_vector(0.0, 0), jobs, job_xr, job_xi);
}
model {
  target += rk[1,1] + bdf[1,1] + adams[1,1] + ck[1,1];
  target += rkt[1,1] + bdft[1,1] + adamst[1,1] + ckt[1,1];
  target += lrk[1,1] + lbdf[1,1] + ladams[1,1];
  target += sum(root) + reduced + static_reduced + sum(mapped);
}
