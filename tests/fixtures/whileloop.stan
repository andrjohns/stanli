data {
  int N;
}
parameters {
  real theta;
}
model {
  int i = 1;
  real acc = 0;
  while (i <= N) {
    acc += i * theta;
    i += 1;
  }
  theta ~ std_normal();
  target += acc;
}
