// STANLI-LIT: XFAIL
// STANLI-LIT-EXPECT: COMPILE_FAIL stanli prepare_data: unsupported statement function FnValidateSizeUnitVector
// STANLI-LIT-DATA: {"K": 3}
data { int K; }
parameters { unit_vector[K] u; }
model { u ~ std_normal(); }
