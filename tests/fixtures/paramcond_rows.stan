// A shape query inside parameter-dependent control flow is compiled by the
// runtime-region register machine. Its answer comes from the logical matrix
// view and remains a constant while the surrounding expression differentiates.
parameters {
  real theta;
  matrix[2, 3] x;
}
model {
  real contribution;
  if (theta > 0) {
    contribution = rows(x) * theta;
  } else {
    contribution = theta;
  }
  target += contribution;
}
