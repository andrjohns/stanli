data {
  array[1] int a;
  matrix[2, 2] m;
}
transformed data {
  array[2] int joined = append_array(a, {2});
  matrix[2, 2] e = matrix_exp(m);
  real td = sum(joined) + sum(e);
}
parameters {
  real theta;
}
model {
  target += td * theta;
}
generated quantities {
  array[2] real g_joined = append_array({theta}, {2.0});
  matrix[2, 2] g_exp = matrix_exp([[theta, 0.0], [0.0, 0.0]]);
  vector[2] product_terms = exp([theta, 2.0]');
  real product = prod(product_terms);
}
