// What would a native vectorized density kernel buy?
//
// stanli's density ops instantiate unmodified stan-math prim templates on
// `rvar`, the recording scalar (runtime/kernels/densities.cpp). That is
// what makes the whole stan-math density library work without porting a
// single function, and it is where a vectorized model spends nearly all
// its gradient time. This measures the ceiling for replacing that with our
// own math, the way runtime/kernels/mixture.cpp replaced the legacy
// log_sum_exp: the same value and partials, written as Eigen array
// expressions.
//
// A: the real density path (OP_NORMAL_LPDF, vector y and mu, scalar sigma)
// B: the same lp and partials by hand, packetized
#include <stanli/graph.hpp>
#include <stanli/optable.hpp>

#include <Eigen/Dense>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace stanli;
using Arr = Eigen::Array<double, -1, 1>;

template <typename F>
static double time_ns(int64_t n, F&& f) {
  const int reps = (int)std::max<int64_t>(5, 8000000 / (n + 1));
  f();
  auto t0 = std::chrono::steady_clock::now();
  for (int r = 0; r < reps; ++r) f();
  auto t1 = std::chrono::steady_clock::now();
  return std::chrono::duration<double, std::nano>(t1 - t0).count() / reps / n;
}

int main() {
  // Registers the kernel table.
  {
    Graph g;
    const int a = g.add_slot(1, true), o = g.add_slot(1, false);
    g.add_op(OP_EXP, {a}, o);
    g.result_slot = o;
    Executor warm(std::move(g));
    (void)warm.n_params();
  }
  std::printf("%8s %12s %12s %8s\n", "N", "recorder", "native", "speedup");
  for (int64_t n : {256LL, 4096LL, 65536LL}) {
    std::vector<double> y((size_t)n), mu((size_t)n), sigma{1.3}, lp{0.0};
    std::vector<double> y_adj((size_t)n, 0.0), mu_adj((size_t)n, 0.0),
        sig_adj{0.0}, scratch((size_t)(2 * n + 1), 0.0), out_adj{1.0};
    for (int64_t i = 0; i < n; ++i) {
      y[(size_t)i] = 0.3 * std::sin(0.5 * (double)i);
      mu[(size_t)i] = 0.1 * std::cos(0.3 * (double)i);
    }

    KernelCtx ctx;
    ctx.n_in = 3;
    ctx.in[0] = Desc{y.data(), n};
    ctx.in[1] = Desc{mu.data(), n};
    ctx.in[2] = Desc{sigma.data(), 1};
    ctx.out = Desc{lp.data(), 1};
    ctx.scratch = scratch.data();
    ctx.variant = 0x06;  // y data; mu and sigma active
    ctx.in_adj[0] = Desc{nullptr, n};
    ctx.in_adj[1] = Desc{mu_adj.data(), n};
    ctx.in_adj[2] = Desc{sig_adj.data(), 1};
    ctx.out_adj = 1.0;
    ctx.out_adj_vec = Desc{out_adj.data(), 1};
    const Kernel& k = kernel(OP_NORMAL_LPDF);
    const double rec = time_ns(n, [&] {
      k.forward(ctx);
      k.backward(ctx);
    });

    // The same math, by hand. normal_lpdf(y | mu, sigma) with y data:
    //   z = (y - mu) / sigma;  lp = sum(-0.5 z^2) - N log(sigma) + const
    //   d/dmu_i = z_i / sigma;  d/dsigma = sum(z^2 - 1) / sigma
    Eigen::Map<const Arr> ya(y.data(), n), mua(mu.data(), n);
    Eigen::Map<Arr> dmu(mu_adj.data(), n);
    Arr z(n);
    double value = 0, dsig = 0;
    const double nat = time_ns(n, [&] {
      const double s = sigma[0], inv = 1.0 / s;
      z = (ya - mua) * inv;
      value = -0.5 * (z * z).sum() - (double)n * std::log(s);
      dmu += (z * inv) * ctx.out_adj;
      dsig += ((z * z).sum() - (double)n) * inv * ctx.out_adj;
    });

    std::printf("%8lld %11.2fns %11.2fns %7.2fx\n", (long long)n, rec, nat,
                rec / nat);
    std::printf("         (lp %.6g vs %.6g, sink %g)\n", lp[0], value, dsig);
  }
  return 0;
}
