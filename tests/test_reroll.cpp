// Re-roll pass: unrolled scalar-loop regions collapse to vector ops with
// gradients preserved (up to summation order, 1e-12 rel).
#include <stanrt/graph.hpp>
#include <stanrt/optable.hpp>
#include <stanrt/reroll.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

static int failures = 0;
static void expect(const char* what, bool ok) {
  if (!ok) { ++failures; std::printf("FAIL %s\n", what); }
}
static void expect_close(const char* what, double got, double want) {
  const double rel =
      std::abs(got - want) / std::max(std::abs(want), 1e-300);
  if (!(rel < 1e-12)) {
    ++failures;
    std::printf("FAIL %-24s got %.17g want %.17g rel %.2e\n", what, got,
                want, rel);
  }
}

using namespace stanrt;
using Fills = std::vector<std::pair<int, std::vector<double>>>;

// Executes gradient at fixed params; returns {lp, grads...}.
static std::vector<double> run_grad(Graph g, const Fills& fills) {
  Executor ex(std::move(g));
  for (const auto& f : fills) {
    double* p = ex.value_ptr(f.first);
    for (size_t j = 0; j < f.second.size(); ++j) p[j] = f.second[j];
  }
  for (int64_t i = 0; i < ex.n_params(); ++i)
    ex.params_data()[i] = 0.2 + 0.1 * (i % 3);
  std::vector<double> out(1 + ex.n_params());
  out[0] = ex.gradient(out.data() + 1);
  return out;
}

// radon shape: mu = vector intermediate written by an op; per lane
// {INDEX(mu,n); NORMAL(y_const_n, idx, sigma)}, lp -> target term.
// y consts deliberately share slots (dedup pool) between lanes 1 and 5.
static void test_radon_shape() {
  const int L = 8;
  Graph g;
  Fills fills;
  const int alpha = g.add_slot(1, true);
  const int sigma = g.add_slot(1, true);
  const int base = g.add_slot(L, false);
  g.add_op(OP_REP_VEC, {alpha}, base);  // makes base a written slot
  std::vector<int> yconst(L);
  for (int n = 0; n < L; ++n) {
    if (n == 5) { yconst[n] = yconst[1]; continue; }  // dedup'd pool
    yconst[n] = g.add_slot(1, false);
    fills.emplace_back(yconst[n], std::vector<double>{0.25 * n - 1.0});
  }
  std::vector<int> terms;
  for (int n = 0; n < L; ++n) {
    const int idx = g.add_slot(1, false);
    g.add_op(OP_INDEX, {base}, idx, {n});
    const int lp = g.add_slot(1, false);
    const int id = g.add_op(OP_NORMAL_LPDF, {yconst[n], idx, sigma}, lp);
    g.ops[id].variant = 0x06;
    terms.push_back(lp);
  }
  // Reference BEFORE the pass (reduce terms via chained ADD_N).
  Graph ref = g;
  {
    int acc = terms[0];
    for (int n = 1; n < L; ++n) {
      const int s = ref.add_slot(1, false);
      ref.add_op(OP_ADD_N, {acc, terms[n]}, s);
      acc = s;
    }
    ref.result_slot = acc;
  }
  const std::vector<double> want = run_grad(std::move(ref), fills);

  std::vector<int> tt = terms;
  Fills f2 = fills;
  RerollStats st = reroll(g, f2, tt);
  expect("radon regions==1", st.regions == 1);
  // 1 REP_VEC survives; 8 INDEX + 8 NORMAL collapse to 1 NORMAL.
  expect("radon ops==2", g.ops.size() == 2);
  expect("radon one term", tt.size() == 1);
  expect("radon vec y filled", f2.size() == fills.size() + 1);
  g.result_slot = tt[0];
  const std::vector<double> got = run_grad(std::move(g), f2);
  expect("radon sizes", got.size() == want.size());
  for (size_t i = 0; i < want.size() && i < got.size(); ++i)
    expect_close(("radon v" + std::to_string(i)).c_str(), got[i], want[i]);
}

int main() {
  test_radon_shape();
  if (failures) { std::printf("%d failures\n", failures); return 1; }
  std::printf("test_reroll OK\n");
  return 0;
}
