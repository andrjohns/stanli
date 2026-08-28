// A generated quantity the optimizer folds to a constant: at --O1 the
// FnWriteParam's argument becomes the literal itself, so the column name
// must come from output_vars instead of the (gone) variable reference.
data {
  matrix[3, 2] rectangular;
}
parameters {
  real x;
}
model {
  x ~ normal(0, 1);
}
generated quantities {
  real z;
  vector[2] extracted;
  z = 3;
  extracted = diagonal(rectangular);
}
