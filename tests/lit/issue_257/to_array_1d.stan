// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
parameters { vector[3] y; }
model {
  array[3] real a = to_array_1d(y);
  y ~ std_normal();
  target += a[1];
}
