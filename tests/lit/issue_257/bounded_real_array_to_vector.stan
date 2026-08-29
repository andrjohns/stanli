// STANLI-LIT: XFAIL
// STANLI-LIT-EXPECT: COMPILE_FAIL stanli compile: binding current: array kind and shape id disagree
parameters { array[2] real<lower=0, upper=1> start; }
transformed parameters {
  vector[2] current;
  current = to_vector(start);
}
model { start ~ beta(2, 2); }
