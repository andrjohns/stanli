// An integer assigned inside parameter-dependent control flow. The region
// compiles it as a compile-time value, so a fold is sound only where the
// assignment certainly runs: `n` is assigned before the break, in a loop
// that runs once, and the lowering takes the folded value back for the
// statements after the region. `k` is declared and assigned inside the
// region, where the fold applies to exactly the reads it should.
parameters {
  real theta;
}
model {
  int n = 1;
  real z = 0;
  for (i in 1 : 1) {
    n = 2;
    if (theta > 0) break;
    int k = 3;
    k = k + 1;
    z += k * theta;
  }
  target += n * theta + z;
}
