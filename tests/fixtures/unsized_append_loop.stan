data {
  array[2] int selected;
}
parameters {
  array[3] vector[2] x;
}
model {
  array[1] int baseline = {0};
  for (i in append_array(baseline, selected))
    target += (i + 1) * x[1][1];

  array[1] vector[2] head = {x[1]};
  array[2] vector[2] tail = {x[2], x[3]};
  array[3] vector[2] joined = append_array(head, tail);
  target += joined[1][1] + 2 * joined[2][2] + 3 * joined[3][1];
}
