data {
  vector[3] e;
  array[3] real ea;
}
parameters {
  vector[3] v;
  array[3] real va;
}
model {
  target += sum(pow(v, e));
  target += sum(pow(va, ea));
  target += sum(pow(v, 2));
}
