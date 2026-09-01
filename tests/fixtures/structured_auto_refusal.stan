data { int<lower=0> N; }
parameters { real theta; }
model {
  vector[N] values = rep_vector(theta, N);
  real state = theta;
  for (i in 2:N) {
    int j = 0;
    while (j < 0) {
      j += 1;
    }
    state += sum(values[2:i]);
  }
  target += state;
}
