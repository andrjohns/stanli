functions {
  vector residual(real t, vector y, vector yp, real rate, array[] real x_r,
                  array[] int x_i) {
    return yp - rep_vector(rate + 0 * sum(x_r) + 0 * sum(x_i), rows(y));
  }
}

transformed data {
  array[1] real ts = {0.2};
  array[1] real x_r = {1.0};
  array[1] int x_i = {1};
}

parameters {
  real gate;
  real initial;
  real rate;
}

model {
  vector[1] y0 = [initial]';
  vector[1] yp0 = [rate]';
  target += -0.5 * (square(gate) + square(initial) + square(rate));
  target += dae(residual, y0, yp0, 0, ts, rate, x_r, x_i)[1, 1];
  if (gate > 0) {
    target += dae_tol(residual, y0, yp0, 0, ts, 1e-10, 1e-10, 100000, rate, x_r,
                      x_i)[1, 1];
  }
}
