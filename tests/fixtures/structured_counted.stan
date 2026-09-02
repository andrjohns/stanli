data { int<lower=0> N; }
parameters { real theta; real beta; }
model {
  real state = theta;
  for (i in 1:N) state = tanh(state * beta + theta + 0.001 * i);
  target += state;
}
