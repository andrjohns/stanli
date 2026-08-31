// unsupported_minmax_expression, generalized: `min`/`max` of a contiguous
// `v[lo:hi]` range slice (not just a bare vector variable) must still lower
// to the native OP_EXTREMA_VEC opcode, with a working reverse pass, for
// both a vector and a row-vector argument.
parameters {
  vector[4] x;
  row_vector[4] r;
}
transformed parameters {
  real vspan = max(x[1 : 3]) - min(x[1 : 3]);
  real rspan = max(r[2 : 4]) - min(r[2 : 4]);
}
model {
  x ~ std_normal();
  r ~ std_normal();
  target += normal_lpdf(vspan | 0, 1);
  target += normal_lpdf(rspan | 0, 1);
}
