data { int<lower=0> N; }
parameters { real theta; real beta; }
model {
  matrix[2, 2] state = rep_matrix(theta, 2, 2);
  for (i in 1:N) {
    int row = 1 + i % 2;
    state[row, 2] = sin(state[row, 2] + beta);
    state = state * 0.8;
  }
  target += sum(state);
}
