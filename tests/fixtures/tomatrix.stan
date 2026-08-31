data {
  vector[6] v;
  array[2, 3] real ar;
}
transformed data {
  matrix[2, 3] a = to_matrix(v, 2, 3);
  matrix[2, 3] b = to_matrix(ar);
  row_vector[6] w = to_row_vector(a);
}
parameters {
  vector[6] x;
}
model {
  matrix[3, 2] c = to_matrix(x, 3, 2);
  target += sum(a * c);
  target += sum(b);
  target += dot_product(x, to_vector(b));
  target += sum(w);
  target += x' * to_vector(a);
}
generated quantities {
  matrix[2, 3] converted = to_matrix(ar);
}
