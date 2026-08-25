data {
  int N;
  int S;
  array[N, S] int idx;
  array[N] int n;
}
parameters {
  real theta;
}
model {
  theta ~ std_normal();
  for (i in 1 : N) {
    array[n[i]] int row = idx[i, 1 : n[i]];
    for (k in 1 : n[i]) target += row[k] * theta;
  }
}
