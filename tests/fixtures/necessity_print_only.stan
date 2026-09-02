// A parameter-dependent effect with no numeric live-out. The necessity
// island must survive lowering solely because its taken arm can print.
parameters {
  real x;
}
model {
  if (x > 0) print("print-only x=", x);
}
