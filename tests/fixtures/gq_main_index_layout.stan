parameters {
  array[2, 2] vector[5] av;
  array[3] matrix[2, 2] am;
}
model {
  for (i in 1:2)
    for (j in 1:2)
      av[i, j] ~ normal(0, 1);
  for (i in 1:3)
    to_vector(am[i]) ~ normal(0, 1);
}
generated quantities {
  real nested_range_prod = prod(av[2, 1, 2:5]);
  real matrix_cell_min = min(am[:, 1, 1]);
}
