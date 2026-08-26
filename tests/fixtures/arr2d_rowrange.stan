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
  // Accumulated into one term on purpose: six `target +=` terms would roll
  // into a SUM_VEC and reassociate the target sum against passes_off.
  real acc = 0;
  for (i in 1 : N) {
    array[n[i]] int row = idx[i, 1 : n[i]];
    for (k in 1 : n[i]) acc += row[k] * theta;
  }
  theta ~ std_normal();
  target += acc;
}
