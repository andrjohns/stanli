parameters {
  matrix[2, 3] a;
}
model {
  matrix[2, 2] gram = tcrossprod(a);
  to_vector(a) ~ normal(0, 1);
  to_vector(gram) ~ normal(0, 1);
}
