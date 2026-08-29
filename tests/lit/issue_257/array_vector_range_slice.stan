// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
// STANLI-LIT-DATA: {"K": 2, "TT": 4}
data { int K; int TT; }
parameters { array[K] vector[TT] eta; }
model {
  for (j in 1:K) {
    eta[j, 1] ~ std_normal();
    eta[j, 2:TT] ~ normal(eta[j, 1:(TT - 1)], 1);
  }
}
