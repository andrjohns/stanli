// Build smoke test: vendored stan-math compiles, links, and computes.
#include <stan/math.hpp>
#include <cmath>
#include <cstdio>

int main() {
  const double lp = stan::math::normal_lpdf<false>(1.0, 0.0, 1.0);
  const double want = -0.5 - 0.5 * std::log(2.0 * M_PI);
  if (lp != want) {
    std::printf("FAIL: normal_lpdf(1,0,1) = %.17g, want %.17g\n", lp, want);
    return 1;
  }
  std::printf("smoke OK\n");
  return 0;
}
