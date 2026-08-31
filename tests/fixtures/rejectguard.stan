// unsupported_FnReject, verbatim: reject() with a literal-only message
// inside a UDF's data-dependent branch, used from both log_prob and
// generated quantities. The register machine that compiles the
// data-dependent `if` now has a REJECT instruction for exactly this case
// (no runtime value in the message, so nothing needs interpolating into
// the thrown string at forward time).
functions {
  real checked(real x) {
    if (x < 0)
      reject("negative input");
    return x;
  }
}
parameters {
  real z;
}
model {
  target += normal_lpdf(checked(z) | 0, 1);
}
generated quantities {
  real draw = checked(z);
}
