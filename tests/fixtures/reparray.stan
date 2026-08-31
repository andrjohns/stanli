data {
  int<lower=1> N;
}
parameters {
  vector[3] a;
  real s;
}
model {
  array[N] vector[3] tiled = rep_array(a, N);
  array[2, 2] real grid = rep_array(s, 2, 2);
  for (n in 1 : N) {
    target += sum(tiled[n]);
  }
  target += grid[1, 1] + grid[2, 2];
}
generated quantities {
  array[N] vector[3] draw = rep_array(a, N);
}
