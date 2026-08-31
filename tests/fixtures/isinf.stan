data {
  real finite_x;
}
transformed data {
  int a = is_inf(finite_x);
  int b = is_inf(1.0 / 0.0);
  int c = is_nan(finite_x);
  int d = is_nan(0.0 / 0.0);
}
parameters {
  real theta;
}
model {
  theta ~ normal(a + b + c + d, 1);
}
