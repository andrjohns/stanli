data {
  int<lower=0> n;
  int<lower=0, upper=1> flag;
}
parameters {
  matrix[n, 2] M;
  real theta;
}
model {
  real scale = 0.0;
  if (rows(M) == 0 || flag == 1)
    scale += 1.0;
  if (rows(M) == 0 || theta > 0.0)
    scale += 10.0;
  else
    scale += 20.0;
  target += scale * theta;
}
