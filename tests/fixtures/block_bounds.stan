data {
  int row;
  int col;
  int nr;
  int nc;
}
parameters {
  matrix[2, 3] x;
}
model {
  target += sum(block(x, row, col, nr, nc));
}
