// An RNG region every one of whose inputs is data. Constant folding runs an
// op once at compile time and replaces it with its result, which would turn
// two draws from the caller's stream into one number baked into the graph.
// island_has_effect is what keeps a region carrying an effect out of that
// pass, so the two draws stay two draws.
data {
  real<lower=0> rate;
}
generated quantities {
  real d1 = 0;
  real d2 = 0;
  int i = 1;
  while (i <= 1) {
    d1 = exponential_rng(rate);
    d2 = exponential_rng(rate);
    i += 1;
  }
}
