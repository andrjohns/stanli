// STANLI-LIT: XFAIL
// STANLI-LIT-EXPECT: OK
// STANLI-LIT-DATA: {"mu": [0, 0], "S": [[1, 0], [0, 1]]}
data { vector[2] mu; cov_matrix[2] S; }
parameters { vector[3] y; }
model { y ~ multi_normal(mu, S); }
