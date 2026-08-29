parameters {
  vector[3] y;
  vector[1] singleton;
}
model {
  y ~ std_normal();
  singleton ~ std_normal();
  target += prod(y) + min(y) + max(y) + sd(y) + variance(y);
  target += sd(singleton) + variance(singleton);
}
