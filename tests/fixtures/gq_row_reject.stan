// Generated quantities that fail on most draws: [mu, 1 - mu] is a simplex
// only while mu stays in [0, 1].
parameters {
  real mu;
}
model {
  mu ~ normal(0, 1);
}
generated quantities {
  int k = categorical_rng([mu, 1 - mu]');
}
