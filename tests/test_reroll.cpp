// Re-roll pass: unrolled scalar-loop regions collapse to vector ops with
// gradients preserved (up to summation order, 1e-12 rel).
#include <stanli/compile.hpp>
#include <stanli/graph.hpp>
#include <stanli/optable.hpp>
#include <stanli/reroll.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
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

using namespace stanli;
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
  RerollStats st = reroll(g, f2, tt, {});
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

// arK shape: per lane {INDEX(beta,k), MUL, ADD} x K then NORMAL.
// beta INDEX ops are lane-invariant (same idata) -> hoisted once.
// MUL second args are per-lane consts (the lag values).
static void test_ark_shape() {
  const int L = 6, K = 2;
  Graph g;
  Fills fills;
  const int alpha = g.add_slot(1, true);
  const int beta = g.add_slot(K, true);
  const int sigma = g.add_slot(1, true);
  auto cslot = [&](double v) {
    const int s = g.add_slot(1, false);
    fills.emplace_back(s, std::vector<double>{v});
    return s;
  };
  std::vector<std::vector<int>> lag(K, std::vector<int>(L));
  std::vector<int> yobs(L);
  for (int l = 0; l < L; ++l) {
    for (int k = 0; k < K; ++k) lag[k][l] = cslot(0.3 * l + 0.1 * k);
    yobs[l] = cslot(0.5 * l - 0.7);
  }
  std::vector<int> terms;
  for (int l = 0; l < L; ++l) {
    int mu = alpha;
    for (int k = 0; k < K; ++k) {
      const int bk = g.add_slot(1, false);
      g.add_op(OP_INDEX, {beta}, bk, {k});
      const int prod = g.add_slot(1, false);
      g.add_op(OP_MUL, {bk, lag[k][l]}, prod);
      const int acc = g.add_slot(1, false);
      g.add_op(OP_ADD, {mu, prod}, acc);
      mu = acc;
    }
    const int lp = g.add_slot(1, false);
    const int id = g.add_op(OP_NORMAL_LPDF, {yobs[l], mu, sigma}, lp);
    g.ops[id].variant = 0x86;  // propto + mu,sigma active, like real arK
    terms.push_back(lp);
  }
  Graph ref = g;
  {
    int acc = terms[0];
    for (int l = 1; l < L; ++l) {
      const int s = ref.add_slot(1, false);
      ref.add_op(OP_ADD_N, {acc, terms[l]}, s);
      acc = s;
    }
    ref.result_slot = acc;
  }
  const std::vector<double> want = run_grad(std::move(ref), fills);

  std::vector<int> tt = terms;
  Fills f2 = fills;
  RerollStats st = reroll(g, f2, tt, {});
  expect("ark regions==1", st.regions == 1);
  // 2 hoisted INDEX + 2 vec MUL + 2 vec ADD + 1 vec NORMAL = 7 ops.
  expect("ark ops==7", g.ops.size() == 7);
  expect("ark one term", tt.size() == 1);
  g.result_slot = tt[0];
  const std::vector<double> got = run_grad(std::move(g), f2);
  expect("ark sizes", got.size() == want.size());
  for (size_t i = 0; i < want.size() && i < got.size(); ++i)
    expect_close(("ark v" + std::to_string(i)).c_str(), got[i], want[i]);
}

// (a) cross-lane dependence: lane l reads lane l-1's output (a recurrence
// on parameters). Must NOT vectorize.
static void test_bail_recurrence() {
  const int L = 6;
  Graph g;
  Fills fills;
  const int sigma = g.add_slot(1, true);
  int prev = g.add_slot(1, true);  // x0 param
  std::vector<int> terms;
  for (int l = 0; l < L; ++l) {
    const int nx = g.add_slot(1, false);
    g.add_op(OP_MUL, {prev, sigma}, nx);
    const int lp = g.add_slot(1, false);
    const int id = g.add_op(OP_NORMAL_LPDF, {nx, prev, sigma}, lp);
    g.ops[id].variant = 0x06;
    terms.push_back(lp);
    prev = nx;  // <- lane l+1 reads lane l's out
  }
  std::vector<int> tt = terms;
  const size_t before = g.ops.size();
  RerollStats st = reroll(g, fills, tt, {});
  expect("recurrence not rerolled", st.regions == 0);
  expect("recurrence ops unchanged", g.ops.size() == before);
}

// (b) density out consumed by another op (gauss_mix shape): must bail.
static void test_bail_nonterm_density() {
  const int L = 6;
  Graph g;
  Fills fills;
  const int mu = g.add_slot(1, true);
  const int sigma = g.add_slot(1, true);
  auto cslot = [&](double v) {
    const int s = g.add_slot(1, false);
    fills.emplace_back(s, std::vector<double>{v});
    return s;
  };
  std::vector<int> terms;
  for (int l = 0; l < L; ++l) {
    const int lp = g.add_slot(1, false);
    const int id = g.add_op(OP_NORMAL_LPDF, {cslot(0.1 * l), mu, sigma}, lp);
    g.ops[id].variant = 0x06;
    const int doubled = g.add_slot(1, false);
    g.add_op(OP_ADD, {lp, lp}, doubled);  // lp escapes into an op
    terms.push_back(doubled);
  }
  std::vector<int> tt = terms;
  const size_t before = g.ops.size();
  RerollStats st = reroll(g, fills, tt, {});
  expect("nonterm density not rerolled", st.regions == 0);
  expect("nonterm ops unchanged", g.ops.size() == before);
}

// (c) partial-range INDEX progression (0..L-1 over a longer base): bail.
static void test_bail_partial_range() {
  const int L = 6;
  Graph g;
  Fills fills;
  const int alpha = g.add_slot(1, true);
  const int sigma = g.add_slot(1, true);
  const int base = g.add_slot(L + 3, false);  // longer than lane count
  g.add_op(OP_REP_VEC, {alpha}, base);
  auto cslot = [&](double v) {
    const int s = g.add_slot(1, false);
    fills.emplace_back(s, std::vector<double>{v});
    return s;
  };
  std::vector<int> terms;
  for (int l = 0; l < L; ++l) {
    const int idx = g.add_slot(1, false);
    g.add_op(OP_INDEX, {base}, idx, {l});
    const int lp = g.add_slot(1, false);
    const int id = g.add_op(OP_NORMAL_LPDF, {cslot(0.2 * l), idx, sigma}, lp);
    g.ops[id].variant = 0x06;
    terms.push_back(lp);
  }
  std::vector<int> tt = terms;
  const size_t before = g.ops.size();
  RerollStats st = reroll(g, fills, tt, {});
  expect("partial range not rerolled", st.regions == 0);
  expect("partial ops unchanged", g.ops.size() == before);
}

// (d) STANLI_NO_REROLL disables the pass.
static void test_env_disable() {
  setenv("STANLI_NO_REROLL", "1", 1);
  const int L = 6;
  Graph g;
  Fills fills;
  const int mu = g.add_slot(1, true);
  const int sigma = g.add_slot(1, true);
  std::vector<int> terms;
  for (int l = 0; l < L; ++l) {
    const int c = g.add_slot(1, false);
    fills.emplace_back(c, std::vector<double>{0.1 * l});
    const int lp = g.add_slot(1, false);
    const int id = g.add_op(OP_NORMAL_LPDF, {c, mu, sigma}, lp);
    g.ops[id].variant = 0x06;
    terms.push_back(lp);
  }
  std::vector<int> tt = terms;
  const size_t before = g.ops.size();
  RerollStats st = reroll(g, fills, tt, {});
  expect("env disables", st.regions == 0);
  expect("env ops unchanged", g.ops.size() == before);
  unsetenv("STANLI_NO_REROLL");
}

// (e) first lane anomalous (its y is an op output, not a const): the pass
// must skip lane 0 and still re-roll lanes 1..L-1 on the second attempt.
static void test_first_lane_anomalous() {
  const int L = 8;
  Graph g;
  Fills fills;
  const int mu = g.add_slot(1, true);
  const int sigma = g.add_slot(1, true);
  // lane 0's observation comes from an op, not the const pool
  const int y0 = g.add_slot(1, false);
  g.add_op(OP_MUL, {mu, sigma}, y0);
  std::vector<int> terms;
  for (int l = 0; l < L; ++l) {
    int y;
    if (l == 0) {
      y = y0;
    } else {
      y = g.add_slot(1, false);
      fills.emplace_back(y, std::vector<double>{0.1 * l});
    }
    const int lp = g.add_slot(1, false);
    const int id = g.add_op(OP_NORMAL_LPDF, {y, mu, sigma}, lp);
    g.ops[id].variant = 0x06;
    terms.push_back(lp);
  }
  Graph ref = g;
  {
    int acc = terms[0];
    for (int l = 1; l < L; ++l) {
      const int s = ref.add_slot(1, false);
      ref.add_op(OP_ADD_N, {acc, terms[l]}, s);
      acc = s;
    }
    ref.result_slot = acc;
  }
  const std::vector<double> want = run_grad(std::move(ref), fills);

  std::vector<int> tt = terms;
  Fills f2 = fills;
  RerollStats st = reroll(g, f2, tt, {});
  expect("anomalous regions==1", st.regions == 1);
  // MUL + lane-0 NORMAL survive scalar; lanes 1..7 collapse to 1 vec op.
  expect("anomalous ops==3", g.ops.size() == 3);
  expect("anomalous terms==2", tt.size() == 2);
  int acc = tt[0];
  for (size_t k = 1; k < tt.size(); ++k) {
    const int s = g.add_slot(1, false);
    g.add_op(OP_ADD_N, {acc, tt[(int)k]}, s);
    acc = s;
  }
  g.result_slot = acc;
  const std::vector<double> got = run_grad(std::move(g), f2);
  expect("anomalous sizes", got.size() == want.size());
  for (size_t i = 0; i < want.size() && i < got.size(); ++i)
    expect_close(("anom v" + std::to_string(i)).c_str(), got[i], want[i]);
}

// (f) block-structured region (rats_model shape, time-major data): the
// INDEX idata is a 0..B-1 progression that restarts every block. The pass
// must split the run at block boundaries and re-roll every block.
static void test_block_structured() {
  const int B = 5;   // lanes per block (== param vector length)
  const int NB = 3;  // blocks
  Graph g;
  Fills fills;
  const int theta = g.add_slot(B, true);
  const int sigma = g.add_slot(1, true);
  std::vector<int> terms;
  for (int b = 0; b < NB; ++b) {
    for (int l = 0; l < B; ++l) {
      const int y = g.add_slot(1, false);
      fills.emplace_back(y, std::vector<double>{0.2 * (b * B + l) - 0.5});
      const int idx = g.add_slot(1, false);
      g.add_op(OP_INDEX, {theta}, idx, {l});  // restarts every block
      const int lp = g.add_slot(1, false);
      const int id = g.add_op(OP_NORMAL_LPDF, {y, idx, sigma}, lp);
      g.ops[id].variant = 0x06;
      terms.push_back(lp);
    }
  }
  Graph ref = g;
  {
    int acc = terms[0];
    for (size_t k = 1; k < terms.size(); ++k) {
      const int s = ref.add_slot(1, false);
      ref.add_op(OP_ADD_N, {acc, terms[k]}, s);
      acc = s;
    }
    ref.result_slot = acc;
  }
  const std::vector<double> want = run_grad(std::move(ref), fills);

  std::vector<int> tt = terms;
  Fills f2 = fills;
  RerollStats st = reroll(g, f2, tt, {});
  expect("block regions==3", st.regions == NB);
  expect("block ops==3", g.ops.size() == (size_t)NB);  // 1 vec NORMAL each
  expect("block terms==3", tt.size() == (size_t)NB);
  int acc = tt[0];
  for (size_t k = 1; k < tt.size(); ++k) {
    const int s = g.add_slot(1, false);
    g.add_op(OP_ADD_N, {acc, tt[k]}, s);
    acc = s;
  }
  g.result_slot = acc;
  const std::vector<double> got = run_grad(std::move(g), f2);
  expect("block sizes", got.size() == want.size());
  for (size_t i = 0; i < want.size() && i < got.size(); ++i)
    expect_close(("block v" + std::to_string(i)).c_str(), got[i], want[i]);
}

// ---- end to end through compile_model ------------------------------------

static std::string slurp(const char* p) {
  std::ifstream f(p);
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

static std::vector<double> e2e_grad(const char* sexp, const char* json,
                                    size_t* n_ops) {
  DataMap data = DataMap::from_json(json);
  CompiledModel cm = compile_model(slurp(sexp), data);
  *n_ops = cm.graph.ops.size();
  Executor ex(std::move(cm.graph));
  cm.bind(ex);
  for (int64_t i = 0; i < ex.n_params(); ++i)
    ex.params_data()[i] = 0.1 + 0.05 * (i % 7) - 0.15 * (i % 3);
  std::vector<double> out(1 + ex.n_params());
  out[0] = ex.gradient(out.data() + 1);
  return out;
}

static void test_e2e_fixtures() {
  const char* rdata =
      "{\"N\":16,\"x\":[0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0,1.1,1.2,"
      "1.3,1.4,1.5,1.6],"
      "\"y\":[1.1,0.9,1.3,0.7,1.0,1.2,0.8,1.05,0.95,1.15,0.85,1.0,1.1,"
      "0.92,1.08,0.98]}";
  const char* adata =
      "{\"K\":2,\"T\":12,\"y\":[0.3,0.5,0.2,0.6,0.4,0.55,0.35,0.45,0.5,"
      "0.42,0.48,0.44]}";
  struct Case {
    const char* sexp;
    const char* json;
    const char* name;
  };
  const Case cases[] = {
      {"tests/fixtures/rloop.tmir.sexp", rdata, "rloop"},
      {"tests/fixtures/arloop.tmir.sexp", adata, "arloop"},
  };
  for (const Case& c : cases) {
    size_t ops_unrolled = 0, ops_rerolled = 0;
    setenv("STANLI_NO_REROLL", "1", 1);
    const std::vector<double> want = e2e_grad(c.sexp, c.json, &ops_unrolled);
    unsetenv("STANLI_NO_REROLL");
    const std::vector<double> got = e2e_grad(c.sexp, c.json, &ops_rerolled);
    expect((std::string(c.name) + " sizes").c_str(),
           got.size() == want.size());
    for (size_t i = 0; i < want.size() && i < got.size(); ++i)
      expect_close((std::string(c.name) + " v" + std::to_string(i)).c_str(),
                   got[i], want[i]);
    std::printf("%s: %zu ops -> %zu ops\n", c.name, ops_unrolled,
                ops_rerolled);
    expect((std::string(c.name) + " shrinks 4x").c_str(),
           ops_rerolled < ops_unrolled / 4);
  }
}

// (e) a lane output that is a graph root the pass is not told about. The
// executor reads jacobian slots and constrained-parameter views directly,
// with no consuming op, so `uses` cannot see them: a region that folds one
// away would silently stop writing a slot something still reads. Same
// graph twice, the second time declaring one lane's intermediate a root.
static void test_bail_extra_root() {
  const auto build = [](Graph& g, Fills& fills, std::vector<int>& terms) {
    const int L = 6;
    const int alpha = g.add_slot(1, true);
    const int sigma = g.add_slot(1, true);
    std::vector<int> scaled(L);
    auto cslot = [&](double v) {
      const int s = g.add_slot(1, false);
      fills.emplace_back(s, std::vector<double>{v});
      return s;
    };
    for (int l = 0; l < L; ++l) {
      scaled[l] = g.add_slot(1, false);
      g.add_op(OP_MUL, {alpha, cslot(0.3 + 0.1 * l)}, scaled[l]);
      const int lp = g.add_slot(1, false);
      const int id =
          g.add_op(OP_NORMAL_LPDF, {cslot(0.2 * l), scaled[l], sigma}, lp);
      g.ops[id].variant = 0x06;
      terms.push_back(lp);
    }
    return scaled;
  };

  {  // control: nothing else reads the intermediates, so it re-rolls
    Graph g;
    Fills fills;
    std::vector<int> tt;
    build(g, fills, tt);
    RerollStats st = reroll(g, fills, tt, {});
    expect("extra-root control rerolls", st.regions == 1);
  }
  {  // one lane's intermediate is a root: the region must not fold it away
    Graph g;
    Fills fills;
    std::vector<int> tt;
    const std::vector<int> scaled = build(g, fills, tt);
    const size_t before = g.ops.size();
    RerollStats st = reroll(g, fills, tt, {scaled[2]});
    expect("extra root not rerolled", st.regions == 0);
    expect("extra root ops unchanged", g.ops.size() == before);
  }
}

int main() {
  test_radon_shape();
  test_ark_shape();
  test_bail_recurrence();
  test_bail_nonterm_density();
  test_bail_partial_range();
  test_env_disable();
  test_first_lane_anomalous();
  test_block_structured();
  test_bail_extra_root();
  test_e2e_fixtures();
  if (failures) { std::printf("%d failures\n", failures); return 1; }
  std::printf("test_reroll OK\n");
  return 0;
}
