functions {
  real portable_helper_lpdf(real x) {
    return normal_lpdf(x | 0, 1);
  }
}
