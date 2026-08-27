// log1p_exp inside a parameter-dependent region. The opcode carries the
// derivative stan-math precomputes for its own reverse rule -- the input's
// inv_logit -- which is also what OP_LOG1P_EXP carries on the graph side,
// so the region's generated backward, the var replay and the graph all
// agree to the bit.
parameters {
  real theta;
}
model {
  if (theta > 0) {
    target += log1p_exp(2 * theta);
  } else {
    target += log1p_exp(theta);
  }
}
