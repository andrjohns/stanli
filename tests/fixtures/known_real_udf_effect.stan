functions {
  real descend_with_print(real x) {
    if (x > 0) return descend_with_print(x - 1);
    print("recursive base");
    return x;
  }
}

transformed data {
  real known = 2.5;
}

parameters {
  real probe;
}

model {
  probe ~ std_normal();
  if (probe > 100)
    print("known effectful recursive real: ", descend_with_print(known));
}
