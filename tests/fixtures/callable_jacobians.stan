functions {
  real all_jacobians_jacobian(vector v, vector cv, matrix m,
                              array[] vector av, array[] matrix am,
                              array[] vector acv) {
    real z = 0;
    z += sum(lower_bound_jacobian(v, -2));
    z += sum(upper_bound_jacobian(v, 2));
    z += sum(lower_upper_bound_jacobian(v, -2, 2));
    z += sum(offset_multiplier_jacobian(v, 0.5, 1.5));
    z += sum(ordered_jacobian(v));
    z += sum(positive_ordered_jacobian(v));
    z += sum(simplex_jacobian(v));
    z += sum(stochastic_column_jacobian(m));
    z += sum(stochastic_row_jacobian(m));
    z += sum(sum_to_zero_jacobian(v));
    z += sum(sum_to_zero_jacobian(m));
    z += sum(unit_vector_jacobian(v));
    z += sum(cholesky_factor_corr_jacobian(v, 3));
    z += sum(corr_matrix_jacobian(v, 3));
    z += sum(cov_matrix_jacobian(v, 2));
    z += sum(cholesky_factor_cov_jacobian(cv, 3, 2));
    // Representative array overloads for each result geometry. The
    // Jacobian applies to every array element even though the value side
    // below deliberately reads only one.
    z += sum(simplex_jacobian(av)[1]);
    z += sum(stochastic_column_jacobian(am)[1]);
    z += sum(corr_matrix_jacobian(av, 3)[1]);
    z += sum(cholesky_factor_cov_jacobian(acv, 3, 2)[1]);
    return z;
  }
}
parameters {
  real probe;
  vector[3] v;
  vector[5] cv;
  matrix[2, 2] m;
  array[2] vector[3] av;
  array[2] matrix[2, 2] am;
  array[2] vector[5] acv;
}
transformed parameters {
  real graph_jacobians = all_jacobians_jacobian(v, cv, m, av, am, acv);
  real branch_jacobians = 0;
  if (probe > 0)
    branch_jacobians = all_jacobians_jacobian(v, cv, m, av, am, acv);
}
model {
  target += graph_jacobians + branch_jacobians;
  probe ~ normal(0, 1);
  v ~ normal(0, 1);
  cv ~ normal(0, 1);
  to_vector(m) ~ normal(0, 1);
  for (a in av) a ~ normal(0, 1);
  for (a in am) to_vector(a) ~ normal(0, 1);
  for (a in acv) a ~ normal(0, 1);
}
