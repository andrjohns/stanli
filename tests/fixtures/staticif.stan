// Model-block if on data indexed by the enclosing loop variable
// (the M0/Mb/Mh/Mt capture-recapture pattern).
data {
  int<lower=0> M;
  array[M] int<lower=0> s;
}
parameters {
  real<lower=0, upper=1> p;
}
model {
  for (i in 1 : M) {
    if (s[i] > 0) {
      target += binomial_lpmf(s[i] | 5, p);
    } else {
      target += bernoulli_lpmf(0 | p);
    }
  }
}
