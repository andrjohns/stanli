// Calling `noisy` from another UDF leaves it an uninlined UserDefined call,
// so lowering meets the effect as a compile-time int demand rather than as
// an inlined print statement.
functions {
  array[] int noisy(data array[] int x) {
    print("int effect");
    return x;
  }
  int pick(data array[] int x) {
    return noisy(x)[1];
  }
}
data {
  array[1] int x_i;
}
parameters {
  real q;
}
model {
  target += bernoulli_lpmf(pick(x_i) | inv_logit(q));
}
