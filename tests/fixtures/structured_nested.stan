functions {
  real advance_state(real state, real theta, real beta) {
    int j = 0;
    real next = state;
    while (j < 3) {
      if (theta > 0) next = inv_logit(next * beta + theta);
      else next = inv_logit(next * beta - theta);
      j += 1;
    }
    return next;
  }
}
data { int<lower=0> N; }
parameters { real theta; real beta; }
model {
  real state = theta;
  for (i in 1:N) {
    state = advance_state(state, theta, beta);
  }
  target += state;
}
