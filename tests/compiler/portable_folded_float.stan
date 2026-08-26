parameters {
  real theta;
}
model {
  target += 0.1 + 0.2;
  theta ~ std_normal();
}
