data {
  array[1] int d;
}
parameters {
  real theta;
}
model {
  if (theta > 0) {
    for (i in append_array({1}, d))
      target += i * theta;
    for (x in {[1.0, 2.0], [3.0, 4.0]})
      target += sum(x) * theta;
  }
}
