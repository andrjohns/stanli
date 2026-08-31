// A parameter's size computed by a transformed-data for-loop accumulator
// (`sumnt2 += ...`), not a bare data value. The interpreter that runs
// transformed data used to lose the running total's int-ness on the first
// loop iteration (a whole-variable reassignment came back through real
// arithmetic), so the parameter's declared size looked like an unknown
// runtime value instead of the computed constant it is.
data {
  int<lower=1> nots;
  array[nots] int<lower=1> nts;
}
transformed data {
  int sumnt2 = 0;
  for (i in 1 : nots)
    sumnt2 += nts[i] * nts[i];
}
parameters {
  vector[sumnt2] x;
}
model {
  x ~ normal(0, 1);
}
