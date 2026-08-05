// Index forms on parameter-carrying values: gather by a data int array,
// Between reads/writes, matrix row/column slices, column writes.
data {
  int<lower=1> N;
  array[N] int<lower=1, upper=3> idx;
}
parameters {
  vector[3] v;
  matrix[2, 3] M;
}
model {
  vector[N] g;
  vector[2] w;
  matrix[2, 3] L;
  g = v[idx];
  w[1 : 2] = v[2 : 3];
  L[ : , 2] = w;
  target += normal_lpdf(g | 0, 1);
  target += normal_lpdf(v[1 : 2] | 0, 2);
  target += normal_lpdf(M[1] | 0, 1);
  target += normal_lpdf(M[ : , 3] | 0, 3);
  target += normal_lpdf(L[ : , 2] | 0, 2);
  target += normal_lpdf(w | 1, 1);
}
