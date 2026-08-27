// A matrix row inside a parameter-dependent region. Column-major storage
// puts a row's elements `rows` apart, so the region copies them into a run
// of their own and the reduction that follows walks that run in ascending
// order -- which is the order stan-math reduces a strided row in, a
// non-contiguous Eigen block having no packet access.
parameters {
  matrix[3, 4] m;
  real theta;
}
model {
  if (theta > 0) {
    target += sum(square(m[2, ]));
  } else {
    target += theta;
  }
}
