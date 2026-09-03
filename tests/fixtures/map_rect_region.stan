functions {
  vector mapped(vector shared, vector job, array[] real x_r,
                array[] int x_i) {
    return [shared[1] + job[1] + sum(x_r) + sum(x_i),
            shared[2] + job[2]]';
  }
}

transformed data {
  array[2, 2] real x_r = {{1, 2}, {3, 4}};
  array[2, 1] int x_i = {{5}, {6}};
}

parameters {
  real gate;
  real x;
}

model {
  target += -0.5 * (square(gate) + square(x));
  vector[2] shared = [x, 2 * x]';
  array[2] vector[2] jobs = {[3 * x, 4 * x]', [5 * x, 6 * x]'};
  vector[4] direct = map_rect(mapped, shared, jobs, x_r, x_i);
  target += direct[1] + 2 * direct[2] + 4 * direct[3] + 8 * direct[4];
  if (gate > 0) {
    vector[4] result = map_rect(mapped, shared, jobs, x_r, x_i);
    target += result[1] + 2 * result[2] + 4 * result[3] + 8 * result[4];
  }
}
