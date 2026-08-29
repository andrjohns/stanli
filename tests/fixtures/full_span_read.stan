// A full-span read is the identity. O1 folds every All index away and leaves
// an Indexed node carrying no indices at all.
parameters {
  matrix[3, 2] m;
  vector[4] v;
}
model {
  target += sum(m[:, :]);
  target += 2 * sum(v[:]);
}
