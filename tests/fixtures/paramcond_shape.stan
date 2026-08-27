// Shape queries in the integer positions inside parameter-dependent
// control flow: declared extents, an integer local, a loop bound. The
// register-machine region answers each from the logical view, which is
// fixed even where the value is a parameter and the branch is not.
parameters {
  real theta;
  matrix[2, 3] x;
  array[4] real a;
}
model {
  if (theta > 0) {
    matrix[rows(x), cols(x)] y = x;
    int k = num_elements(y) + cols(y);
    real acc = 0;
    for (i in 1 : size(a)) acc += a[i];
    target += k * theta + acc;
  } else {
    target += theta;
  }
}
