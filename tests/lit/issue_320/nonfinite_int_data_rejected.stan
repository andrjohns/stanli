// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: int variable contained non-int values
// STANLI-LIT-DATA: {"N": Infinity}
data { int N; }
parameters { real mu; }
model { mu ~ normal(0, 1); }
