parameters {
  real theta;
}
model {
  for (i in {0, 2, 4})
    target += (i + 1) * theta;
}
