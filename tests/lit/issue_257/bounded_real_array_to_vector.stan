// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
parameters { array[2] real<lower=0, upper=1> start; }
transformed parameters {
  vector[2] current;
  current = to_vector(start);
}
model { start ~ beta(2, 2); }
