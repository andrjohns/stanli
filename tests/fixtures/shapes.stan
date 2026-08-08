// Every shape 0.4.0 got wrong, in one model, checked as one gradient.
//
// The three focused fixtures (truncvec, mnarr, dirvec) each pin one
// construct and say which fix broke when they fail. This one is the
// integration check: the constructs share a graph, a data block and a
// parameter vector here, so it also covers what none of them can, which is
// one construct disturbing another. Every fix has to hold at once for the
// gradient to come out.
//
// What each statement is for:
//
//   t ~ normal(mu, sigma) T[0, 10]
//     Vectorized truncation over a scalar location. The normalizer is
//     FnLength(t) * log_diff_exp(...), and FnLength is a compiler-internal
//     rather than a stan-library name.
//
//   t ~ normal(theta, 1) T[0, 10]
//     Vectorized truncation over a container location. stanc3 loops over
//     the elements and hoists the literal scale into a temporary it
//     declares (Unsized UReal), which carries no size expression.
//
//   y ~ multi_normal(m, Sigma)
//     array[N] vector[K] data into a vectorized multivariate density. Data
//     is stored first index fastest, the way a matrix is; the kernel needs
//     element n contiguous in K. Getting this wrong is silent.
//
//   p ~ dirichlet(a)
//     Vectorized dirichlet over an array of simplexes. The kernel read the
//     whole slot as one theta and threw on the length against alpha.
//
// N and K differ on purpose. A square case hides a transpose.
//
// Putting the last two together buys something neither has alone. The
// layout bug is silent in isolation: multi_normal takes permuted data and
// returns a plausible number. Here the same permutation reaches dirichlet,
// where the rows have to sum to 1, so it stops being silent and throws
// `probabilities is not a valid simplex, sum = 0.7`. Reverting any one of
// the three fixes fails this test, each with its own signature.
data {
  int N;
  int K;
  vector[N] t;
  array[N] vector[K] y;
  array[N] vector[K] p;
  matrix[K, K] Sigma;
}
parameters {
  real mu;
  vector[N] theta;
  real<lower=0> sigma;
  vector[K] m;
  vector<lower=0>[K] a;
}
model {
  t ~ normal(mu, sigma) T[0, 10];
  t ~ normal(theta, 1) T[0, 10];
  y ~ multi_normal(m, Sigma);
  p ~ dirichlet(a);
}
