data {
  int N;
}
parameters {
  real theta;
}
model {
  int i = 1;
  real acc = 0;
  real position = 0;
  real step = 1.0 / ceil(N / 2.0);
  while (i <= N) {
    acc += i * theta;
    i += 1;
  }
  while (position < 1) {
    acc += theta;
    position += step;
  }
  theta ~ std_normal();
  target += acc;
}
