// A UDF local whose every write is skipped at runtime.
functions {
  array[] int vecequals(array[] int a, int test) {
    array[size(a)] int check;
    for (i in 1:size(check)) check[i] = (a[i] == test);
    return check;
  }
  array[] int whichequals(array[] int b, int test) {
    array[size(b)] int check = vecequals(b, test);
    array[sum(check)] int which;
    int counter = 1;
    for (i in 1:size(b)) {
      if (check[i] == 1) {
        which[counter] = i;
        counter += 1;
      }
    }
    return which;
  }
}
data {
  int<lower=0> N;
  array[N] int input;
}
parameters {
  real theta;
}
model {
  array[size(whichequals(input, 9))] int selected = whichequals(input, 9);
  theta ~ normal(0, 1);
  for (i in 1 : size(selected)) target += selected[i] * theta;
}
