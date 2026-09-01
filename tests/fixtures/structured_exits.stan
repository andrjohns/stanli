data { int<lower=0> N; }
parameters { real theta; real beta; }
model {
  real state = theta;
  for (i in 1:N) {
    if (theta > 0 && i == 2) continue;
    for (j in -1:3) {
      if (j > 1 && theta < 0) break;
      state = inv_logit(state * beta + theta);
    }
    if (i > 2 && theta > 0) break;
  }
  target += state;
}
