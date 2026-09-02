functions {
  real descend(real x) {
    if (x > 0) return descend(x - 1);
    return x;
  }
}

transformed data {
  real known = 40.5;
}

parameters {
  real probe;
}

model {
  probe ~ std_normal();
  if (probe > 100) print("known deep recursive real: ", descend(known));
}
