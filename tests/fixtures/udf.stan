// Transformed-data user-defined function (the obtain_adjustments pattern:
// loops, range slices, 2-D writes, early metadata fns, return), plus
// ternary selection on data conditions in the model block.
functions {
  matrix scale2(matrix W) {
    matrix[2, cols(W)] adj;
    adj[1, 1] = 0;
    adj[2, 1] = 1;
    for (k in 2 : cols(W)) {
      adj[1, k] = mean(W[1 : rows(W), k]);
      adj[2, k] = sd(W[1 : rows(W), k]) * 2;
    }
    return adj;
  }
}
data {
  int<lower=1> N;
  int<lower=1> K;
  matrix[N, K] W;
  int<lower=0, upper=1> flag;
}
transformed data {
  matrix[2, K] adj = scale2(W);
}
parameters {
  vector[K] beta;
}
model {
  target += normal_lpdf(beta | adj[1, 2], adj[2, 2]);
  target += normal_lpdf(beta[1] | flag == 1 || K > 5 ? 0.5 : -0.5, 1);
  target += normal_lpdf(beta[2] | flag == 1 ? beta[1] : 0.0, 1);
}
