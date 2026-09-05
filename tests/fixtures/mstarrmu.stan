data {
  int N;
  int K;
  array[N] vector[K] y;
  matrix[K, K] S;
}
parameters {
  array[N] vector[K] Mu;
}
model {
  target += multi_student_t_lpdf(y | 3.0, Mu, S);
}
