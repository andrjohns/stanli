// --O1 lowers an early UDF return to an assignment plus Break inside a
// single-iteration sentinel loop. Ordinary graph lowering must stop that
// loop without dropping the function's return value.
functions {
  real first_positive(vector x) {
    for (i in 1:size(x))
      if (x[i] > 0) return x[i];
    return -1;
  }
}
data {
  vector[3] x;
}
parameters {
  real theta;
}
model {
  theta ~ std_normal();
  target += first_positive(x) * theta;
}
