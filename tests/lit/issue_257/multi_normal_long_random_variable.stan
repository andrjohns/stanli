// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: COMPILE_FAIL stanli compile: multi_normal_lpdf: random variable length 3 is not a positive multiple of matrix size 2
// STANLI-LIT-DATA: {"mu": [0, 0], "S": [[1, 0], [0, 1]]}
data { vector[2] mu; cov_matrix[2] S; }
parameters { vector[3] y; }
model { y ~ multi_normal(mu, S); }
