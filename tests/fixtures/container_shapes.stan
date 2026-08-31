transformed data {
  array[2, 3] int a = {{1, 2, 3}, {4, 5, 6}};
  matrix[2, 3] weights = to_matrix(a);
  array[2] real one = ones_array(2);
  matrix[0, 0] empty_matrix = to_matrix(rep_array(1.0, 0, 3));
  one[1] = 0.5;
}
parameters {
  matrix[2, 3] m;
}
model {
  array[2, 1, 3] matrix[2, 3] copies = rep_array(m, 2, 1, 3);
  array[0] matrix[2, 3] empty = rep_array(m, 0);
  array[2] matrix[2, 3] reversed = reverse({m, 2 * m});
  target += sum(copies[2, 1, 3] .* weights);
  target += one[1] * m[1, 2];
  target += reversed[1][2, 3] + reversed[2][1, 2];
  target += size(empty) + rows(empty_matrix) + cols(empty_matrix);
}
generated quantities {
  array[2, 1, 3] matrix[2, 3] copies = rep_array(m, 2, 1, 3);
  array[2] matrix[2, 3] reversed = reverse({m, 2 * m});
  matrix[0, 0] empty_converted = to_matrix(rep_array(1.0, 0, 3));
}
