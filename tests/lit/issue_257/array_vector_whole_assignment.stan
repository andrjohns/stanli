// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
// STANLI-LIT-DATA: {"T": 3}
data { int T; }
parameters { vector[2] phi0; }
transformed parameters {
  array[T] vector[2] mu;
  for (t in 1:T) mu[t, ] = phi0;
}
model { phi0 ~ std_normal(); }
