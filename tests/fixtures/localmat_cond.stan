// A data-only `if` whose operands are not in the interpreter's frame.
//
// `matout` is declared uninitialized and filled element by element, so the
// lowering builds it in the graph and drops it from the data environment.
// The second loop then reads it back in a condition. The MIR marks that
// condition DataOnly -- every operand is data at this call site -- but the
// interpreter cannot fold it, because the values are in a graph slot rather
// than its environment. Only a region program can read them there.
//
// The holes matter: element writes leave the untouched entries at CmdStan's
// NaN, and `is_nan` on them is what the condition asks about.
functions {
  matrix fillholes(data matrix matin, data real x) {
    matrix[rows(matin), cols(matin)] matout;
    for (ri in 1 : rows(matin)) {
      for (ci in 1 : cols(matin)) {
        if (matin[ri, ci] > 0) matout[ri, ci] = x * matin[ri, ci];
      }
    }
    for (ri in 1 : rows(matin)) {
      for (ci in 1 : cols(matin)) {
        if (is_nan(matout[ri, ci]) && !is_nan(matin[ri, ci])) {
          matout[ri, ci] = matin[ri, ci];
        }
      }
    }
    return matout;
  }
}
data {
  matrix[2, 2] m;
}
parameters {
  real theta;
}
model {
  theta ~ std_normal();
  target += sum(fillholes(m, 2.0)) * theta;
}
