functions {
  vector returned_gather(vector x, array[] int idx) {
    // Keep the UDF visible in optimized MIR. Its by-value vector return is an
    // owning materialization even though the returned expression is a gather.
    array[2] matrix[2, 2] uninlined_shape_guard;
    return x[idx];
  }
}
data {
  array[5] int<lower=1, upper=5> idx;
}
parameters {
  vector[5] x;
  array[2] vector[5] av;
}
model {
  x ~ normal(0, 1);
  for (n in 1 : 2) av[n] ~ normal(0, 1);
}
generated quantities {
  vector[5] initialized = x[idx];
  vector[5] assigned;
  assigned = x[idx];
  real initialized_prod = prod(initialized);
  real assigned_prod = prod(assigned);
  real returned_prod = prod(returned_gather(x, idx));
  real inner_prod = prod(av[2]);
  real inner_tail_prod = prod(av[2, 2 : 5]);
  real exp_gather_min = min(exp(x[idx]));
}
