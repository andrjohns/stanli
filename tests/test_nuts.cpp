// NUTS end to end through the vendored stan sampler driving the executor's
// gradient. Statistical checks with fixed seeds.
#include "models.hpp"

#include <stanli/nuts.hpp>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

static int failures = 0;
static void expect_in(const std::string& what, double got, double lo,
                      double hi) {
  if (!(got >= lo && got <= hi)) {
    ++failures;
    std::printf("FAIL %-18s got %.6g want in [%g, %g]\n", what.c_str(), got,
                lo, hi);
  }
}

int main() {
  using namespace stanli;

  // ---- 10-dim standard normal: mean 0, sd 1 ------------------------------
  {
    const int D = 10;
    Graph g;
    const int x = g.add_slot(D, true);
    const int zero = g.add_slot(1, false);
    const int one = g.add_slot(1, false);
    const int lp = g.add_slot(1, false);
    g.add_op(OP_NORMAL_LPDF, {x, zero, one}, lp);
    g.result_slot = lp;
    Executor ex(std::move(g));
    ex.value_ptr(zero)[0] = 0.0;
    ex.value_ptr(one)[0] = 1.0;

    NutsConfig cfg;
    cfg.seed = 20260804;
    cfg.warmup = 1000;
    cfg.samples = 2000;
    auto draws = run_nuts(ex, cfg);
    if ((int)draws.size() != cfg.samples) {
      std::printf("FAIL draw count %zu\n", draws.size());
      return 1;
    }
    for (int d = 0; d < D; ++d) {
      double m = 0, m2 = 0;
      for (const auto& q : draws) m += q[d];
      m /= draws.size();
      for (const auto& q : draws) m2 += (q[d] - m) * (q[d] - m);
      const double sd = std::sqrt(m2 / (draws.size() - 1));
      expect_in("norm mean[" + std::to_string(d) + "]", m, -0.15, 0.15);
      expect_in("norm sd[" + std::to_string(d) + "]", sd, 0.85, 1.15);
    }
  }

  // ---- eight schools: posterior locations, no NaNs -----------------------
  {
    auto m = testmodels::eight_schools();
    Executor ex(std::move(m.graph));
    testmodels::fill_eight_schools_data(m, ex);

    NutsConfig cfg;
    cfg.seed = 8675309;
    cfg.warmup = 1000;
    cfg.samples = 2000;
    cfg.delta = 0.9;  // funnel-adjacent geometry, adapt a little tighter
    auto draws = run_nuts(ex, cfg);

    double mu_mean = 0, tau_mean = 0;
    int nan_count = 0;
    for (const auto& q : draws) {
      for (double v : q)
        if (std::isnan(v)) ++nan_count;
      mu_mean += q[0];
      tau_mean += std::exp(q[1]);
    }
    mu_mean /= draws.size();
    tau_mean /= draws.size();
    expect_in("es nan_count", nan_count, 0, 0);
    expect_in("es mu mean", mu_mean, 2.5, 6.5);
    expect_in("es tau mean", tau_mean, 2.0, 6.0);
  }

  if (failures == 0) std::printf("test_nuts OK\n");
  return failures == 0 ? 0 : 1;
}
