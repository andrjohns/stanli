functions {
  real scaled_head(vector v) {
    int D = rows(v);
    real out;
    if (D == 1) out = v[1];
    else out = sum(v) / D;
    return out;
  }
}
data { int<lower=0> N; }
parameters {
  array[2] vector[1] vl;
  real theta;
}
model {
  target += normal_lpdf(theta | scaled_head(vl[1]), 1);
  target += normal_lpdf(vl[2] | 0, 1);
}
