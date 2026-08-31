data {
  vector[3] base;
}
parameters {
  vector[3] x;
}
model {
  vector[3] a = x .^ base;
  vector[3] b = base .^ x;
  target += sum(a) + sum(b);
}
