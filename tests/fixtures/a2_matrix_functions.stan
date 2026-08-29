parameters {
  vector[4] y;
  vector[2] diagonal;
}
model {
  matrix[2, 2] A = [[exp(y[1]), y[3]], [y[4], exp(y[2])]];
  matrix[2, 2] S = diag_matrix(exp(y[1 : 2]));

  y ~ std_normal();
  diagonal ~ std_normal();
  target += sum(inverse(A));
  target += sum(inverse_spd(S));
  target += log_determinant(A);
  target += quad_form(A, [1.0, 2.0]');
  target += sum(add_diag(A, diagonal));
}
