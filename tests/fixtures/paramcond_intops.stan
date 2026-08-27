// The comparisons and the logical operators, in the integer positions
// inside a parameter-dependent region: a `while` condition, and integer
// locals written as comparisons. Each is a compile-time integer exactly
// when its operands are, and the region compiler answers it that way --
// there is no register-machine opcode that could produce an extent or a
// loop count instead.
data {
  int k;
}
parameters {
  real theta;
}
model {
  if (theta > 0) {
    int hits = 0;
    int i = 1;
    while (i <= 4 && hits < 3) {
      hits += (i != k);
      i += 1;
    }
    int any = !(hits == 0);
    int either = (hits > 2) || (hits == 0);
    int ge = (hits >= 3);
    int lt = (hits < 3);
    target += (hits + any + either + ge + lt) * theta;
  } else {
    target += theta;
  }
}
