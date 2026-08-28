data {
  array[1, 2] int a;
  array[1, 2] int b;
}
parameters {
  real theta;
  vector[3] v;
}
model {
  array[2, 2] int joined_ints = append_array(a, b);
  for (i in joined_ints[2])
    target += i * theta;

  array[0] vector[2] empty;
  array[1] vector[3] one = {v};
  array[1] vector[3] left_empty = append_array(empty, one);
  array[1] vector[3] right_empty = append_array(one, empty);
  target += sum(left_empty[1]) + 2 * sum(right_empty[1]);
}
