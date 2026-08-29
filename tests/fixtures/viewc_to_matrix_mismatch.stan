parameters { vector[6] q; }
model {
  matrix[2, 4] M = to_matrix(q, 2, 4);
  target += sum(M);
}

// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: COMPILE_FAIL stanli compile: to_matrix: requested shape does not match source length
