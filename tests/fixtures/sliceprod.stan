// unsupported_prod, generalized: `prod` of a contiguous `v[lo:hi]` range
// slice (not just a bare vector variable) must still lower to the native
// OP_PROD_VEC opcode, with a working reverse pass, for both a vector and a
// row-vector argument.
parameters {
  vector[4] x;
  row_vector[4] r;
}
transformed parameters {
  real vprod = prod(x[1 : 3]);
  real rprod = prod(r[2 : 4]);
}
model {
  x ~ std_normal();
  r ~ std_normal();
  target += normal_lpdf(vprod | 0, 1);
  target += normal_lpdf(rprod | 0, 1);
}
