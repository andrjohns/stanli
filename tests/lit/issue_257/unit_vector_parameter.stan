// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
// STANLI-LIT-DATA: {"K": 3}
data { int K; }
parameters { unit_vector[K] u; }
model { u ~ std_normal(); }
