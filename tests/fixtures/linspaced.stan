transformed data {
  array[4] int ia = linspaced_int_array(4, 1, 7);
  array[5] int ir = linspaced_int_array(5, 2, 3);
  array[1] int io = linspaced_int_array(1, 3, 9);
  array[3] real ra = linspaced_array(3, -1.5, 2.5);
  vector[4] v = linspaced_vector(4, 0.0, 1.0);
  row_vector[3] rv = linspaced_row_vector(3, 2.0, 8.0);
}
parameters {
  vector[4] x;
  vector[3] y;
}
model {
  target += dot_product(x, v);
  target += rv * y;
  for (n in 1 : 4) {
    target += ia[n] * x[n];
  }
  for (n in 1 : 3) {
    target += ra[n] * y[n];
  }
  for (n in 1 : 5) {
    target += ir[n];
  }
  target += io[1];
}
