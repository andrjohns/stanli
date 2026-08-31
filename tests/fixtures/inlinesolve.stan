// The reproducer for unsupported_undeclared_inline_solve, verbatim. The
// while loop's early-return branch (`if (N == 0) return 0;`) is dead for
// every input in the fixture data (N is always >= 1), so the surrounding
// graph lowering constant-folds it away; the runtime-control region that
// the while loop carves out then has to write its own inlined return-temp
// with no live-in to import for it. `y` never reaches target -- lp is
// exactly normal(0,1) on theta -- but the model must still compile, and
// the write_array value math is checked by hand in test_lower.cpp against
// this exact data (N=1, ts=[1], M=1, grid=[0]).
functions {
  int find_interval_elem(real x, vector sorted, int start_ind) {
    int N = size(sorted);
    int left_ind = start_ind;
    int right_ind = N;
    int iter = 1;
    if (N == 0)
      return 0;
    while ((right_ind - left_ind) > 1 && iter < 10) {
      int mid_ind = (left_ind + right_ind) %/% 2;
      if (sorted[mid_ind] < x)
        left_ind = mid_ind;
      else
        right_ind = mid_ind;
      iter += 1;
    }
    return left_ind;
  }
  vector rhs(real t, vector y, data vector grid) {
    vector[1] d;
    int idx = find_interval_elem(t, grid, 1);
    d[1] = y[1] + idx;
    return d;
  }
  matrix solve_TKTD_var(vector y0, vector ts, data vector grid) {
    array[size(ts)] vector[1] ode_res;
    vector[1] statevars = y0;
    ode_res[1] = y0 + 1e-9 * rhs(ts[1], statevars, grid);
    for (i in 2 : size(ts)) {
      statevars = statevars + rhs(ts[i], statevars, grid) * 0.01;
      ode_res[i] = statevars;
    }
    matrix[size(ts), 1] out;
    for (i in 1 : size(ts)) {
      out[i] = transpose(ode_res[i]);
    }
    return out;
  }
}
data {
  int<lower=1> N;
  vector[N] ts;
  int<lower=1> M;
  vector[M] grid;
}
parameters {
  real theta;
}
transformed parameters {
  matrix[N, 1] y;
  vector[1] y0 = [theta]';
  y[1 : N, 1] = to_vector(solve_TKTD_var(y0, ts, grid));
}
model {
  theta ~ normal(0, 1);
}
