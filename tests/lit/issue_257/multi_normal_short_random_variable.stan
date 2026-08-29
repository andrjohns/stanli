// STANLI-LIT: XFAIL
// STANLI-LIT-EXPECT: CRASH
// STANLI-LIT-DATA: {"mu": [0, 0, 0], "S": [[1, 0, 0], [0, 1, 0], [0, 0, 1]]}
data { vector[3] mu; cov_matrix[3] S; }
parameters { vector[2] y; }
model { y ~ multi_normal(mu, S); }
