// reduce_sum's serial lowering, in the five shapes that can go wrong.
//
// The partial-sum function is reached through a Var, not a call, so the
// `_lupdf` / `_lpdf` spelling at the reduce_sum call site is the only thing
// left saying whether the term is normalized. `partial_sum` is called both
// ways here, and the two must differ by exactly normal's normalizing
// constant.
//
// `bounded` reads x[start:end], so it is wrong unless the rewrite passes the
// 1-based whole-slice bounds.
//
// `never` slices a zero-length array and must return zero without the callee
// running at all -- its body rejects, so a lowering that reached it would
// leave an OP_REJECT in the graph.
//
// `scaled` runs in transformed data, which is evaluated at load by the MIR
// interpreter and never sees the op graph. That engine carries its own copy
// of the same single-call rewrite, and this is what exercises it.
functions {
  real partial_sum_lpdf(array[] real y_slice, int start, int end, real mu,
                        real sigma) {
    return normal_lupdf(y_slice | mu, sigma);
  }
  real bounded_lpdf(array[] real y_slice, int start, int end, array[] real x,
                    real b) {
    return normal_lupdf(y_slice | to_vector(x[start:end]) * b, 1.0);
  }
  real scaled(array[] real s, int start, int end, real k) {
    real acc = 0;
    for (i in 1:size(s)) acc += s[i] * k * (start + end);
    return acc;
  }
  real never_lpdf(array[] real y_slice, int start, int end, real mu) {
    reject("reduce_sum lowered an empty slice's callee");
    return normal_lupdf(y_slice | mu, 1.0);
  }
}
data {
  int<lower=0> N;
  array[N] real y;
  array[N] real x;
}
transformed data {
  real scale = reduce_sum(scaled, x, 1, 2.0);
}
parameters {
  real mu;
  real<lower=0> sigma;
  real b;
}
model {
  array[0] real nothing;
  int grainsize = 1;
  // The same term unnormalized and normalized, then the bounds reader
  // and the empty slice.
  target += reduce_sum(partial_sum_lupdf, y, grainsize, mu, sigma);
  target += reduce_sum(partial_sum_lpdf, y, grainsize, mu, sigma);
  // A pure integer grainsize may use operations without a graph kernel.
  target += reduce_sum_static(bounded_lupdf, y, divide(N + 2, 2), x, b);
  target += reduce_sum(never_lupdf, nothing, 1, mu);
  mu ~ normal(scale, 1.0);
}
