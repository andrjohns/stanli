data {
  int N;
  array[3] int idx;
}
parameters {
  vector[3] raw;
}
model {
  vector[N] x = rep_vector(0, N);
  x[idx] = raw;
  raw ~ std_normal();
  target += sum(x .* x);
}
