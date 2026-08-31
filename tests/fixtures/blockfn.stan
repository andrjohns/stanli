data {
  matrix[4, 5] m;
}
transformed data {
  matrix[2, 3] td = block(m, 2, 3, 2, 3);
}
parameters {
  matrix[4, 5] p;
}
model {
  matrix[2, 3] q = block(p, 2, 3, 2, 3);
  matrix[3, 2] c = block(p, 1, 4, 3, 2);
  target += sum(q .* td);
  target += sum(c);
}
