// STANLI-LIT: XFAIL
// STANLI-LIT-EXPECT: COMPILE_FAIL stanli compile: assignment logical view mismatch for inline_pick_return_sym8__
// STANLI-LIT-DATA: {"K": 2, "g": [2, 2, 2]}
functions {
  matrix pick(matrix y, int k, array[] int ref, int value) {
    int n = 0;
    for (ii in 1:size(ref)) if (ref[ii] == value) n += 1;
    matrix[n, k] res;
    int jj = 1;
    for (ii in 1:size(ref))
      if (ref[ii] == value) {
        for (kk in 1:k) res[jj, kk] = y[ii, kk];
        jj += 1;
      }
    return res;
  }
}
data { int K; array[3] int g; }
parameters { matrix[3, K] X; }
model {
  to_vector(X) ~ std_normal();
  target += sum(pick(X, K, g, 1) * rep_vector(1.0, K));
}
