// beta[:, 1, 1]: an outer array range kept in full, with fixed row/column
// indices into each element's matrix. Every parameter also has a proper
// prior so the model itself is identified (the reproducer this covers,
// unsupported_index_expression_array3d, only has the sum() term and is
// improper by construction, same as CmdStan on the same file).
parameters {
  array[2] matrix[2, 2] beta;
}
model {
  for (i in 1 : 2)
    to_vector(beta[i]) ~ normal(0, 1);
  target += sum(beta[ : , 1, 1]);
}
