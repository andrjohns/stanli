data {
  matrix[3, 3] d;
  vector[3] dv;
}
parameters {
  matrix[3, 3] a;
  matrix[3, 2] b;
  vector[3] v;
}
model {
  matrix[2, 2] q = quad_form_sym(a + a', b);
  target += q[1, 1] - 0.7 * q[2, 1] + 1.3 * q[1, 2] + 0.4 * q[2, 2];
  target += 0.9 * quad_form_sym(a + a', v);
  target += 1.7 * quad_form_sym(d, dv);
  target += 0.3 * quad_form_sym(d, v);
}
