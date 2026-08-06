// Can we get the vectorized speed WITHOUT porting stan-math functions?
//
// Three ways to obtain value + partials for the same call, all of them
// reusing stan-math, plus the hand-written version as the ceiling:
//
//   recorder  today's density path: prim template on rvar, no tape
//   varmat    nested replay on var_value<VectorXd> -- SoA, ONE vari for a
//             whole vector, and the hand-optimized rev overloads that
//             `stanc --O1` exists to reach
//   AoS       nested replay on Matrix<var> -- what the legacy ops do now
//   native    our own math (bench_density.cpp's version), for scale
//
// If varmat lands near native, the speedup needs no ported functions.
#include <stanli/graph.hpp>
#include <stanli/optable.hpp>

#include <stan/math.hpp>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace stanli;

template <typename F>
static double time_ns(int64_t n, F&& f) {
  const int reps = (int)std::max<int64_t>(5, 4000000 / (n + 1));
  f();
  auto t0 = std::chrono::steady_clock::now();
  for (int r = 0; r < reps; ++r) f();
  auto t1 = std::chrono::steady_clock::now();
  return std::chrono::duration<double, std::nano>(t1 - t0).count() / reps / n;
}

int main() {
  {
    Graph g;
    const int a = g.add_slot(1, true), o = g.add_slot(1, false);
    g.add_op(OP_EXP, {a}, o);
    g.result_slot = o;
    Executor warm(std::move(g));
    (void)warm.n_params();
  }

  std::printf("normal_lpdf(y | mu, sigma), y data, mu vector, sigma scalar\n");
  std::printf("%8s %11s %11s %11s %11s\n", "N", "recorder", "varmat", "AoS",
              "native");
  for (int64_t n : {256LL, 4096LL, 65536LL}) {
    std::vector<double> y((size_t)n), mu((size_t)n), sigma{1.3}, lp{0.0};
    std::vector<double> mu_adj((size_t)n, 0.0), sig_adj{0.0},
        scratch((size_t)(2 * n + 1), 0.0), out_adj{1.0};
    for (int64_t i = 0; i < n; ++i) {
      y[(size_t)i] = 0.3 * std::sin(0.5 * (double)i);
      mu[(size_t)i] = 0.1 * std::cos(0.3 * (double)i);
    }
    Eigen::Map<const Eigen::VectorXd> yv(y.data(), n), muv(mu.data(), n);

    KernelCtx ctx;
    ctx.n_in = 3;
    ctx.in[0] = Desc{y.data(), n};
    ctx.in[1] = Desc{mu.data(), n};
    ctx.in[2] = Desc{sigma.data(), 1};
    ctx.out = Desc{lp.data(), 1};
    ctx.scratch = scratch.data();
    ctx.variant = 0x06;
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

    double sink = 0;
    const double vm = time_ns(n, [&] {
      stan::math::nested_rev_autodiff nested;
      stan::math::var_value<Eigen::VectorXd> m(muv);
      stan::math::var s(sigma[0]);
      stan::math::var j = stan::math::normal_lpdf<false>(yv, m, s);
      stan::math::grad(j.vi_);
      sink += m.adj().sum() + s.adj();
    });

    const double aos = time_ns(n, [&] {
      stan::math::nested_rev_autodiff nested;
      Eigen::Matrix<stan::math::var, -1, 1> m(n);
      for (int64_t i = 0; i < n; ++i) m(i) = mu[(size_t)i];
      stan::math::var s(sigma[0]);
      stan::math::var j = stan::math::normal_lpdf<false>(yv, m, s);
      stan::math::grad(j.vi_);
      double acc = s.adj();
      for (int64_t i = 0; i < n; ++i) acc += m(i).adj();
      sink += acc;
    });

    using Arr = Eigen::Array<double, -1, 1>;
    Eigen::Map<const Arr> ya(y.data(), n), mua(mu.data(), n);
    Eigen::Map<Arr> dmu(mu_adj.data(), n);
    Arr z(n);
    const double nat = time_ns(n, [&] {
      const double s = sigma[0], inv = 1.0 / s;
      z = (ya - mua) * inv;
      sink += -0.5 * (z * z).sum() - (double)n * std::log(s);
      dmu += (z * inv);
    });

    std::printf("%8lld %10.2fns %10.2fns %10.2fns %10.2fns  (sink %.3g)\n",
                (long long)n, rec, vm, aos, nat, sink);
  }

  std::printf("\nlog_sum_exp(vector) -- no partials machinery in prim, so"
              " the recorder cannot serve it.\n\"kernel\" is whatever"
              " OP_LOG_SUM_EXP is registered as today.\n");
  std::printf("%8s %11s %11s %11s\n", "N", "kernel", "varmat", "AoS");
  for (int64_t n : {8LL, 256LL, 4096LL}) {
    std::vector<double> x((size_t)n), out{0.0}, adj((size_t)n, 0.0),
        scratch((size_t)n, 0.0), out_adj{1.0};
    for (int64_t i = 0; i < n; ++i) x[(size_t)i] = 0.4 * std::sin(0.9 * i);
    Eigen::Map<const Eigen::VectorXd> xv(x.data(), n);

    KernelCtx ctx;
    ctx.n_in = 1;
    ctx.in[0] = Desc{x.data(), n};
    ctx.out = Desc{out.data(), 1};
    ctx.scratch = scratch.data();
    ctx.in_adj[0] = Desc{adj.data(), n};
    ctx.out_adj = 1.0;
    ctx.out_adj_vec = Desc{out_adj.data(), 1};
    const Kernel& k = kernel(OP_LOG_SUM_EXP);
    const double nat = time_ns(n, [&] {
      k.forward(ctx);
      k.backward(ctx);
    });

    double sink = 0;
    const double vm = time_ns(n, [&] {
      stan::math::nested_rev_autodiff nested;
      stan::math::var_value<Eigen::VectorXd> v(xv);
      stan::math::var j = stan::math::log_sum_exp(v);
      stan::math::grad(j.vi_);
      sink += v.adj().sum();
    });
    const double aos = time_ns(n, [&] {
      stan::math::nested_rev_autodiff nested;
      Eigen::Matrix<stan::math::var, -1, 1> v(n);
      for (int64_t i = 0; i < n; ++i) v(i) = x[(size_t)i];
      stan::math::var j = stan::math::log_sum_exp(v);
      stan::math::grad(j.vi_);
      double acc = 0;
      for (int64_t i = 0; i < n; ++i) acc += v(i).adj();
      sink += acc;
    });
    std::printf("%8lld %10.2fns %10.2fns %10.2fns  (sink %.3g)\n",
                (long long)n, nat, vm, aos, sink);
  }
  return 0;
}
