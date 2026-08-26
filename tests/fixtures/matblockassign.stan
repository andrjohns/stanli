// M[rows, cols] = rhs with a list index. ridx repeats an index so the cell
// it names twice pins CmdStan's last-wins order; the second write is uni x list.
data {
  array[3] int ridx;
  array[2] int cidx;
  matrix[3, 2] b;
}
parameters {
  real theta;
}
model {
  matrix[3, 3] M = rep_matrix(0, 3, 3);
  M[ridx, cidx] = b * theta;
  M[1, cidx] = [7.0, 8.0] * theta;
  theta ~ std_normal();
  target += sum(M);
}
