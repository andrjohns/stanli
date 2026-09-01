// state_probs[1, 1, 1:2]: a full array-index prefix pins one row_vector
// leaf element, then a trailing range reads inside it. Proper priors make
// the model identified (unsupported_index_expression_rowvector, which this
// covers, only has the sum() term and is improper by construction, same as
// CmdStan on the same file).
parameters {
  array[2, 2] row_vector[3] state_probs;
}
model {
  for (i in 1 : 2)
    for (j in 1 : 2)
      state_probs[i, j] ~ normal(0, 1);
  target += sum(state_probs[1, 1, 1 : 2]);
}
