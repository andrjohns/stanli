// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
// STANLI-LIT-DATA: {"J": [2, 3]}
data { array[2] int J; }
transformed data { int total = sum(J); }
parameters { vector[total] lambda; }
model { lambda ~ std_normal(); }
