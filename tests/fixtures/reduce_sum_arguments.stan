// All arguments are evaluated even for an empty slice. The partial body
// itself runs only for a nonempty slice and a positive grainsize.
functions {
  int grain(data int value) {
    print("grain=", value);
    return value;
  }
  real shared(real value, data int refuse) {
    print("shared=", value);
    if (refuse) reject("shared argument rejected");
    return value;
  }
  real partial_lpdf(array[] real slice, int start, int end, real mu) {
    print("partial bounds=", start, ":", end);
    return normal_lupdf(slice | mu, 1);
  }
}
data {
  int<lower=0> N;
  array[N] real y;
  int grainsize;
  int refuse;
  int in_td;
}
transformed data {
  real td_result = 0;
  if (in_td)
    td_result = reduce_sum_static(partial_lpdf, y, grain(grainsize),
                                 shared(0.25, refuse));
}
parameters {
  real mu;
}
model {
  if (!in_td)
    target += reduce_sum(partial_lupdf, y, grain(grainsize),
                         shared(mu, refuse));
  mu ~ normal(td_result, 1);
}
