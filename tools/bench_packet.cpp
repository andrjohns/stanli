// Spike: what does packet (varmat-style) math buy over the scalar-libm,
// sequential-reduction arithmetic the kernels use today for bitwise parity
// with default CmdStan's AoS Matrix<var> paths?
//
// Each pair is the SAME mathematical operation, once as the kernels write
// it now (scalar loop, std:: functions, sequential sum) and once as an
// Eigen array expression, which packetizes. Run at several N because the
// answer is different when the data fits in L1 and when it does not.
#include <Eigen/Dense>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

using Arr = Eigen::Array<double, -1, 1>;

template <typename F>
static double time_ns(int64_t n, F&& f) {
  int reps = (int)std::max<int64_t>(3, 20000000 / (n + 1));
  f();
  auto t0 = std::chrono::steady_clock::now();
  for (int r = 0; r < reps; ++r) f();
  auto t1 = std::chrono::steady_clock::now();
  return std::chrono::duration<double, std::nano>(t1 - t0).count() / reps / n;
}

int main() {
  std::printf("%-16s %8s %10s %10s %7s\n", "op", "N", "scalar", "packet",
              "speedup");
  for (int64_t n : {64LL, 1024LL, 12573LL, 200000LL}) {
    std::vector<double> x((size_t)n), out((size_t)n);
    for (int64_t i = 0; i < n; ++i)
      x[(size_t)i] = -2.0 + 4.0 * (double)(i % 97) / 97.0;
    Eigen::Map<Arr> xa(x.data(), n), oa(out.data(), n);
    double sink = 0;

    const double exp_s = time_ns(n, [&] {
      for (int64_t i = 0; i < n; ++i) out[(size_t)i] = std::exp(x[(size_t)i]);
      sink += out[0];
    });
    const double exp_p = time_ns(n, [&] {
      oa = xa.exp();
      sink += out[0];
    });

    const double log_s = time_ns(n, [&] {
      for (int64_t i = 0; i < n; ++i)
        out[(size_t)i] = std::log(std::abs(x[(size_t)i]) + 1e-3);
      sink += out[0];
    });
    const double log_p = time_ns(n, [&] {
      oa = (xa.abs() + 1e-3).log();
      sink += out[0];
    });

    // inv_logit as constrain.cpp writes it (Eigen's scalar logistic, inf
    // guarded) vs the array expression.
    const double il_s = time_ns(n, [&] {
      for (int64_t i = 0; i < n; ++i) {
        const double e = std::exp(x[(size_t)i]);
        out[(size_t)i] = e / (1.0 + e);
      }
      sink += out[0];
    });
    const double il_p = time_ns(n, [&] {
      oa = xa.exp() / (1.0 + xa.exp());
      sink += out[0];
    });

    // Reductions: sequential (what the kernels do, to match Eigen's redux
    // over a non-vectorizable strided expression) vs Eigen's own sum.
    const double sum_s = time_ns(n, [&] {
      double s = 0;
      for (int64_t i = 0; i < n; ++i) s += x[(size_t)i];
      sink += s;
    });
    const double sum_p = time_ns(n, [&] { sink += xa.sum(); });

    std::printf("%-16s %8lld %9.2fns %9.2fns %6.2fx\n", "exp", (long long)n,
                exp_s, exp_p, exp_s / exp_p);
    std::printf("%-16s %8lld %9.2fns %9.2fns %6.2fx\n", "log", (long long)n,
                log_s, log_p, log_s / log_p);
    std::printf("%-16s %8lld %9.2fns %9.2fns %6.2fx\n", "inv_logit",
                (long long)n, il_s, il_p, il_s / il_p);
    std::printf("%-16s %8lld %9.2fns %9.2fns %6.2fx\n", "sum", (long long)n,
                sum_s, sum_p, sum_s / sum_p);
    std::printf("  (sink %g)\n", sink);
  }
  return 0;
}
