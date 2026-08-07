// `~` inside a parameter-dependent region. The region binds every
// argument the same way, so it cannot reproduce which constants the
// propto form drops -- lp would be off by exactly those constants while
// the gradient looked perfect. Lowering must refuse this, and say what
// to write instead.
data { real y; }
parameters { real mu; }
model {
  if (mu > 0) {
    y ~ normal(mu, 1);
  } else {
    y ~ normal(-mu, 1);
  }
}
