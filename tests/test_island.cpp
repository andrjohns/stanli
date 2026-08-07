// Tape islands: a compiled region must match the op-by-op graph it
// replaces (values and gradients), and everything the carver cannot prove
// safe must stay untouched.
#include <stanli/graph.hpp>
#include <stanli/island.hpp>
#include <stanli/optable.hpp>

#include <cmath>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

static int failures = 0;
static void expect(const char* what, bool ok) {
  if (!ok) {
    ++failures;
    std::printf("FAIL %s\n", what);
  }
}
static void expect_close(const std::string& what, double got, double want) {
  const double rel = std::abs(got - want) / std::max(std::abs(want), 1e-300);
  if (!(rel < 1e-12)) {
    ++failures;
    std::printf("FAIL %-24s got %.17g want %.17g rel %.2e\n", what.c_str(),
                got, want, rel);
  }
}

using namespace stanli;
using Fills = std::vector<std::pair<int, std::vector<double>>>;

static std::vector<double> run_grad(Graph g, const Fills& fills) {
  Executor ex(std::move(g));
  for (const auto& f : fills) {
    double* p = ex.value_ptr(f.first);
    for (size_t j = 0; j < f.second.size(); ++j) p[j] = f.second[j];
  }
  for (int64_t i = 0; i < ex.n_params(); ++i)
    ex.params_data()[i] = 0.3 + 0.15 * (i % 4);
  std::vector<double> out(1 + ex.n_params());
  out[0] = ex.gradient(out.data() + 1);
  return out;
}

// A mini HMM forward pass: per step, index the previous state pair, take
// their log-sum-exp, add per-state emission lps (scalar NORMAL, propto
// off), SET_INDEX the new pair into a zero-backed template vector. The
// final step's pair feeds one LSE2 whose out is the target term, which
// ends the region: the pair slots become the island's live-outs.
struct HmmGraph {
  Graph g;
  Fills fills;
  std::vector<int> terms;
  size_t body_ops = 0;
};

static HmmGraph build_hmm(int T) {
  HmmGraph h;
  Graph& g = h.g;
  const int gp0 = g.add_slot(2, true);   // initial log-state
  const int mu = g.add_slot(2, true);
  const int sigma = g.add_slot(1, true);
  const int z2 = g.add_slot(2, false);   // fill-backed template (absorbed)
  h.fills.emplace_back(z2, std::vector<double>{0.0, 0.0});
  auto cslot = [&](double v) {
    const int s = g.add_slot(1, false);
    h.fills.emplace_back(s, std::vector<double>{v});
    return s;
  };
  int gp = gp0;
  const size_t start = g.ops.size();
  for (int t = 0; t < T; ++t) {
    const int y = cslot(0.35 * t - 0.8);
    const int g0 = g.add_slot(1, false);
    g.add_op(OP_INDEX, {gp}, g0, {0});
    const int g1 = g.add_slot(1, false);
    g.add_op(OP_INDEX, {gp}, g1, {1});
    const int s0 = g.add_slot(1, false);
    g.add_op(OP_LSE2, {g0, g1}, s0);
    int next = -1;
    for (int k = 0; k < 2; ++k) {
      const int mk = g.add_slot(1, false);
      g.add_op(OP_INDEX, {mu}, mk, {k});
      const int em = g.add_slot(1, false);
      const int id = g.add_op(OP_NORMAL_LPDF, {y, mk, sigma}, em);
      g.ops[id].variant = 0x06;  // y data, mu/sigma active; propto OFF
      const int nk = g.add_slot(1, false);
      g.add_op(OP_ADD, {s0, em}, nk);
      const int dst = g.add_slot(2, false);
      g.add_op(OP_SET_INDEX, {k == 0 ? z2 : next, nk}, dst, {k});
      next = dst;
    }
    gp = next;
  }
  h.body_ops = g.ops.size() - start;
  const int f0 = g.add_slot(1, false);
  g.add_op(OP_INDEX, {gp}, f0, {0});
  const int f1 = g.add_slot(1, false);
  g.add_op(OP_INDEX, {gp}, f1, {1});
  const int lp = g.add_slot(1, false);
  g.add_op(OP_LSE2, {f0, f1}, lp);
  h.terms.push_back(lp);
  g.result_slot = lp;
  return h;
}

static void test_hmm_parity() {
  HmmGraph ref = build_hmm(8);   // 8*11 = 88 body ops, above threshold
  const std::vector<double> want = run_grad(std::move(ref.g), ref.fills);

  HmmGraph isl = build_hmm(8);
  const size_t before = isl.g.ops.size();
  const int carved = carve_islands(isl.g, isl.fills, isl.terms, {});
  expect("hmm carved==1", carved == 1);
  // The run swallows the two trailing INDEX ops too (in vocab, non-term);
  // their outs feed the term LSE2 outside, so they are the live-outs:
  // island + 2 extractions + the final LSE2.
  expect("hmm ops==4", isl.g.ops.size() == 4);
  expect("hmm shrank", isl.g.ops.size() < before);
  expect("hmm island first", isl.g.ops[0].opcode == OP_ISLAND);
  expect("hmm 3 live-ins", isl.g.ops[0].n_in == 3);
  const std::vector<double> got = run_grad(std::move(isl.g), isl.fills);
  expect("hmm sizes", got.size() == want.size());
  for (size_t i = 0; i < want.size() && i < got.size(); ++i)
    expect_close("hmm v" + std::to_string(i), got[i], want[i]);
}

static void test_short_run_untouched() {
  HmmGraph h = build_hmm(2);  // 22 body ops, under threshold
  const size_t before = h.g.ops.size();
  const int carved = carve_islands(h.g, h.fills, h.terms, {});
  expect("short none carved", carved == 0);
  expect("short ops unchanged", h.g.ops.size() == before);
}

static void test_propto_density_refused() {
  HmmGraph h = build_hmm(8);
  for (auto& op : h.g.ops)
    if (op.opcode == OP_NORMAL_LPDF) op.variant = 0x86;  // propto ON
  const size_t before = h.g.ops.size();
  const int carved = carve_islands(h.g, h.fills, h.terms, {});
  expect("propto none carved", carved == 0);
  expect("propto ops unchanged", h.g.ops.size() == before);
}

static void test_unsupported_op_splits() {
  // POW mid-region splits the run into halves below the threshold.
  HmmGraph h = build_hmm(5);  // 55 body ops
  Graph& g = h.g;
  const size_t mid = g.ops.size() / 2;
  Op pw;
  pw.opcode = OP_POW;
  pw.n_in = 2;
  pw.in[0] = g.ops[mid].in[0];
  pw.in[1] = g.ops[mid].in[0];
  pw.out = g.add_slot(1, false);
  g.ops.insert(g.ops.begin() + (long)mid, pw);
  h.terms.back() = h.g.result_slot;  // unchanged, re-anchor after insert
  const size_t before = g.ops.size();
  const int carved = carve_islands(g, h.fills, h.terms, {});
  expect("split none carved", carved == 0);
  expect("split ops unchanged", g.ops.size() == before);
}

static void test_too_many_live_ins() {
  Graph g;
  Fills fills;
  std::vector<int> ps;
  for (int k = 0; k < 7; ++k) ps.push_back(g.add_slot(1, true));
  int acc = ps[0];
  for (int k = 1; k < 7; ++k) {
    const int s = g.add_slot(1, false);
    g.add_op(OP_ADD, {acc, ps[k]}, s);
    acc = s;
  }
  for (int k = 0; k < 40; ++k) {
    const int s = g.add_slot(1, false);
    g.add_op(OP_TANHV, {acc}, s);
    acc = s;
  }
  const int lp = g.add_slot(1, false);
  g.add_op(OP_SQUARE, {acc}, lp);
  g.result_slot = lp;
  std::vector<int> terms{lp};
  const size_t before = g.ops.size();
  const int carved = carve_islands(g, fills, terms, {});
  expect("livein7 none carved", carved == 0);
  expect("livein7 ops unchanged", g.ops.size() == before);
}

static void test_six_live_ins_ok() {
  Graph g;
  Fills fills;
  std::vector<int> ps;
  for (int k = 0; k < 6; ++k) ps.push_back(g.add_slot(1, true));
  int acc = ps[0];
  for (int k = 1; k < 6; ++k) {
    const int s = g.add_slot(1, false);
    g.add_op(OP_ADD, {acc, ps[k]}, s);
    acc = s;
  }
  for (int k = 0; k < 40; ++k) {
    const int s = g.add_slot(1, false);
    g.add_op(OP_TANHV, {acc}, s);
    acc = s;
  }
  const int lp = g.add_slot(1, false);
  g.add_op(OP_SQUARE, {acc}, lp);
  g.result_slot = lp;
  std::vector<int> terms{lp};

  Graph ref = g;
  const std::vector<double> want = run_grad(std::move(ref), fills);
  const int carved = carve_islands(g, fills, terms, {});
  expect("livein6 carved==1", carved == 1);
  const std::vector<double> got = run_grad(std::move(g), fills);
  for (size_t i = 0; i < want.size() && i < got.size(); ++i)
    expect_close("livein6 v" + std::to_string(i), got[i], want[i]);
}

// A slot PRODUCED BEFORE the region, then read and updated in place inside
// it, and read again after: it is a live-in and a live-out at once. If the
// island's extraction wrote that same slot, its adjoint buffer would hold
// two different quantities at once -- the extraction's backward leaves
// d(lp)/d(slot-after-region) there, and the island's backward then adds
// d(lp)/d(slot-before-region) on top. The producer's backward, which runs
// later in the reverse sweep, would read the sum and double-count. The
// extraction gets a fresh slot instead, so this checks the gradient with
// respect to the producer's own parameter.
static void test_live_in_and_out_slot() {
  Graph g;
  Fills fills;
  const int seedp = g.add_slot(1, true);   // feeds the producer
  const int a = g.add_slot(1, true);
  const int vec = g.add_slot(10, false);
  g.add_op(OP_REP_VEC, {seedp}, vec);      // the producer, before the region
  auto cslot = [&](double v) {
    const int s = g.add_slot(1, false);
    fills.emplace_back(s, std::vector<double>{v});
    return s;
  };
  // Each lane reads vec[k] (the producer's value), and writes vec[k]
  // destructively, exactly as an unrolled `x[k] = f(x[k])` loop lowers.
  for (int k = 0; k < 10; ++k) {
    const int e = g.add_slot(1, false);
    g.add_op(OP_INDEX, {vec}, e, {k});
    const int m = g.add_slot(1, false);
    g.add_op(OP_MUL, {e, a}, m);
    const int s1 = g.add_slot(1, false);
    g.add_op(OP_ADD, {m, cslot(0.2 * k)}, s1);
    const int t = g.add_slot(1, false);
    g.add_op(OP_TANHV, {s1}, t);
    Op si;  // destructive, as make_inplace_updates emits
    si.opcode = OP_SET_INDEX_INPLACE;
    si.n_in = 2;
    si.in[0] = vec;
    si.in[1] = t;
    si.out = vec;
    g.idata_pool.push_back({k});
    si.idata = g.idata_pool.back().data();
    si.n_idata = 1;
    g.ops.push_back(si);
  }
  const int lp = g.add_slot(1, false);
  g.add_op(OP_LOG_SUM_EXP, {vec}, lp);
  g.result_slot = lp;
  std::vector<int> terms{lp};

  Graph ref = g;
  const std::vector<double> want = run_grad(std::move(ref), fills);
  const int carved = carve_islands(g, fills, terms, {});
  expect("liveinout carved==1", carved == 1);
  expect("liveinout vec is live-in",
         g.ops[1].opcode == OP_ISLAND && g.ops[1].n_in >= 1);
  const std::vector<double> got = run_grad(std::move(g), fills);
  for (size_t i = 0; i < want.size() && i < got.size(); ++i)
    expect_close("liveinout v" + std::to_string(i), got[i], want[i]);
}

int main() {
  test_hmm_parity();
  test_live_in_and_out_slot();
  test_short_run_untouched();
  test_propto_density_refused();
  test_unsupported_op_splits();
  test_too_many_live_ins();
  test_six_live_ins_ok();
  if (failures) {
    std::printf("%d failures\n", failures);
    return 1;
  }
  std::printf("test_island: all passed\n");
  return 0;
}
