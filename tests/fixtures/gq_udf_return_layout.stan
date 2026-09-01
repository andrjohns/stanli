functions {
  vector returned_gather(vector x, array[] int idx) {
    return x[idx];
  }
}
data {
  array[5] int<lower=1, upper=5> idx;
}
parameters {
  vector[5] x;
}
model {
  x ~ normal(0, 1);
}
generated quantities {
  real returned_prod = prod(returned_gather(x, idx));
}
