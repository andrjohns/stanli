data {
  real finite_x;
}
transformed data {
  int a = is_inf(finite_x);
  int b = is_inf(1.0 / 0.0);
  int c = is_nan(finite_x);
  int d = is_nan(0.0 / 0.0);
  // Nullary constants stanc will not fold: check they round-trip through
  // is_inf / is_nan to the same integers.
  int e = is_inf(negative_infinity());
  int f = is_inf(positive_infinity());
  int g = is_nan(not_a_number());
  real eps = machine_precision();
  int h = eps > 0.0;
}
parameters {
  real theta;
}
model {
  theta ~ normal(a + b + c + d + e + f + g + h, 1);
}
