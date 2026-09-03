data { int<lower=0> N; }
parameters { real theta; }
model {
  real state = theta;
  int j = 0;
  while (j < N) {
    state = 0.5 * tanh(state + theta);
    j += 1;
  }
  target += state;
}
