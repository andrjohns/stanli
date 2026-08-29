data {
  int N;
  vector[N] times;
  int enabled;
}
parameters {
  real theta;
}
model {
  real previous = 0;
  real current = 1;
  int changed = 0;
  for (n in 1:N) {
    current = times[n];
    changed = enabled ? current != previous : 0;
    previous = current;
  }
  theta ~ std_normal();
  target += changed * theta;
}
