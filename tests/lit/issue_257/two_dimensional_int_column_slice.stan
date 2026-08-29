// STANLI-LIT: XFAIL
// STANLI-LIT-EXPECT: COMPILE_FAIL stanli compile: unsupported int index expression
// STANLI-LIT-DATA: {"winner": [[1, 2], [3, 4], [2, 1]]}
functions {
  real f(vector v, array[] int idx) {
    real s = 0;
    for (i in 1:size(idx)) s += v[idx[i]];
    return s;
  }
}
data { array[3, 2] int winner; }
parameters { vector[4] elo; }
model { elo ~ std_normal(); target += f(elo, winner[, 1]); }
