parameters { real y; }
model {
  vector[0] a;
  a[:] = rep_vector(y, 1);
  target += y - a[1];
}

// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: COMPILE_FAIL stanli compile: assignment width mismatch for a
