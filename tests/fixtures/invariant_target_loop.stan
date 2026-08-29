data {
  int<lower=0> N;
  int<lower=0> M;
  int<lower=0, upper=7> mode;
}
parameters {
  real x;
}
model {
  if (mode == 0) {
    for (n in 1 : N) {
      x ~ normal(0, 1);
    }
  } else if (mode == 1) {
    for (n in 1 : N) {
      x ~ normal(n, 1);
    }
  } else if (mode == 2) {
    real acc = 0;
    for (n in 1 : N) {
      acc = acc + 1;
      x ~ normal(0, 1);
    }
  } else if (mode == 3) {
    for (n in 1 : N) {
      print("invariant target loop");
      x ~ normal(0, 1);
    }
  } else if (mode == 4) {
    for (n in 1 : N) {
      x ~ normal(0, 1);
      reject("invariant target loop");
    }
  } else if (mode == 5) {
    for (n in 1 : N) {
      for (m in 1 : M) {
        x ~ normal(0, 1);
      }
    }
  } else if (mode == 6) {
    for (n in 1 : N) {
      real lane = n;
      x ~ normal(lane, 1);
    }
  } else {
    for (n in 1 : N) {
      real twice_x = 2 * x;
      twice_x ~ normal(0, 1);
      target += 0.25 * x;
    }
  }
}
