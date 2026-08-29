// STANLI-LIT: XFAIL
// STANLI-LIT-EXPECT: COMPILE_FAIL stanli compile: unsupported function to_array_1d
parameters { vector[3] y; }
model {
  array[3] real a = to_array_1d(y);
  y ~ std_normal();
  target += a[1];
}
