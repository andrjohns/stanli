// diagonal on both paths. Inside a parameter-dependent region the register
// program copies the elements into a run of their own -- column-major
// storage puts them rows + 1 apart, and Eigen's diagonal stops at the
// shorter side -- and outside it the graph spells the same extraction as a
// strided slice. Neither existed before: the function was unsupported
// everywhere in the runtime.
parameters {
  matrix[3, 4] m;
  real theta;
}
model {
  if (theta > 0) {
    target += sum(square(diagonal(m)));
  } else {
    target += theta;
  }
  target += diagonal(m)[3];
}
