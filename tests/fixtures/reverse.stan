transformed data {
  vector[3] td = reverse([1.0, 2.0, 3.0]');
  array[3] int ti = reverse({1, 2, 3});
  array[2] vector[2] ta = reverse({[1.0, 2.0]', [3.0, 4.0]'});
}
parameters {
  vector[3] x;
  row_vector[3] rx;
  array[2] vector[2] ax;
}
model {
  array[2] vector[2] rax = reverse(ax);
  target += dot_product(reverse(x), td);
  target += reverse(rx) * td;
  for (n in 1 : 2) {
    target += ti[n] * dot_product(rax[n], ta[n]);
  }
}
