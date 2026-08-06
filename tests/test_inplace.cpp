// In-place functional updates: a chain of OP_SET_INDEX writes into the
// same vector must collapse onto one buffer (O(N) instead of O(N^2)) with
// values and gradients unchanged.
#include <stanli/graph.hpp>
#include <stanli/inplace.hpp>
#include <stanli/optable.hpp>

#include <cmath>
#include <cstdio>
#include <string>
#include <utility>
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
    std::printf("FAIL %-26s got %.17g want %.17g rel %.2e\n", what, got,
                want, rel);
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
    ex.params_data()[i] = 0.2 + 0.1 * (i % 3);
  std::vector<double> out(1 + ex.n_params());
  out[0] = ex.gradient(out.data() + 1);
  return out;
}

// Two gradient evaluations must agree: an in-place chain that wrongly
// mutated a fill-backed slot would drift on the second pass.
static std::vector<double> run_grad_twice(Graph g, const Fills& fills) {
  Executor ex(std::move(g));
  for (const auto& f : fills) {
    double* p = ex.value_ptr(f.first);
    for (size_t j = 0; j < f.second.size(); ++j) p[j] = f.second[j];
  }
  for (int64_t i = 0; i < ex.n_params(); ++i)
    ex.params_data()[i] = 0.2 + 0.1 * (i % 3);
  std::vector<double> first(1 + ex.n_params()), second(1 + ex.n_params());
  first[0] = ex.gradient(first.data() + 1);
  second[0] = ex.gradient(second.data() + 1);
  for (size_t i = 0; i < first.size(); ++i)
    expect_close(("repeat v" + std::to_string(i)).c_str(), second[i],
                 first[i]);
  return second;
}

// radon_county_intercept's shape: per lane
//   mu_next = SET_INDEX(mu_prev, alpha_g + beta*x_n, n)
//   lp_n    = NORMAL(y_n, INDEX(mu_next, n), sigma)
// The read-back makes mu_next's LAST use the next lane's SET_INDEX, so a
// single-use rule would refuse; a last-use rule accepts.
static Graph build_chain(int L, Fills& fills, std::vector<int>& terms,
                         int* n_vec_slots) {
  Graph g;
  const int beta = g.add_slot(1, true);
  const int sigma = g.add_slot(1, true);
  const int mu0 = g.add_slot(L, false);  // declared vector: fill-backed
  fills.emplace_back(mu0, std::vector<double>((size_t)L, 0.0));
  auto cslot = [&](double v) {
    const int s = g.add_slot(1, false);
    fills.emplace_back(s, std::vector<double>{v});
    return s;
  };
  int prev = mu0;
  *n_vec_slots = 1;
  for (int n = 0; n < L; ++n) {
    const int prod = g.add_slot(1, false);
    g.add_op(OP_MUL, {beta, cslot(0.1 * n - 0.3)}, prod);
    const int nxt = g.add_slot(L, false);
    ++*n_vec_slots;
    g.add_op(OP_SET_INDEX, {prev, prod}, nxt, {n});
    const int rd = g.add_slot(1, false);
    g.add_op(OP_INDEX, {nxt}, rd, {n});
    const int lp = g.add_slot(1, false);
    const int id = g.add_op(OP_NORMAL_LPDF, {cslot(0.2 * n), rd, sigma}, lp);
    g.ops[id].variant = 0x06;
    terms.push_back(lp);
    prev = nxt;
  }
  return g;
}

static void reduce_into_result(Graph& g, const std::vector<int>& terms) {
  int acc = terms[0];
  for (size_t k = 1; k < terms.size(); ++k) {
    const int s = g.add_slot(1, false);
    g.add_op(OP_ADD_N, {acc, terms[k]}, s);
    acc = s;
  }
  g.result_slot = acc;
}

static void test_chain_collapses() {
  const int L = 8;
  Fills fills;
  std::vector<int> terms;
  int n_vec = 0;
  Graph g = build_chain(L, fills, terms, &n_vec);
  expect("built L+1 vector slots", n_vec == L + 1);

  Graph ref = g;
  reduce_into_result(ref, terms);
  const std::vector<double> want = run_grad(std::move(ref), fills);

  const int n_inplace = make_inplace_updates(g, /*roots=*/{});
  // Every write but the first (whose source is the fill-backed mu0) can
  // become destructive.
  expect("L-1 writes made in place", n_inplace == L - 1);
  int copies = 0, inplaces = 0;
  for (const Op& op : g.ops) {
    copies += op.opcode == OP_SET_INDEX;
    inplaces += op.opcode == OP_SET_INDEX_INPLACE;
  }
  expect("one copying write left", copies == 1);
  expect("rest in place", inplaces == L - 1);

  reduce_into_result(g, terms);
  const std::vector<double> got = run_grad_twice(std::move(g), fills);
  expect("sizes", got.size() == want.size());
  for (size_t i = 0; i < want.size() && i < got.size(); ++i)
    expect_close(("chain v" + std::to_string(i)).c_str(), got[i], want[i]);
}

// A later reader of the pre-write vector forbids the rewrite: the write is
// no longer the last use.
static void test_bail_later_reader() {
  const int L = 6;
  Fills fills;
  std::vector<int> terms;
  int n_vec = 0;
  Graph g = build_chain(L, fills, terms, &n_vec);
  // Find the second SET_INDEX and read its INPUT vector after the chain.
  int second_in = -1, seen = 0;
  for (const Op& op : g.ops)
    if (op.opcode == OP_SET_INDEX && ++seen == 2) second_in = op.in[0];
  expect("found second write", second_in >= 0);
  const int late = g.add_slot(1, false);
  g.add_op(OP_INDEX, {second_in}, late, {0});  // reads a mid-chain value
  const int lp = g.add_slot(1, false);
  // slot 1 is build_chain's sigma parameter (positive at the eval point).
  const int id = g.add_op(OP_NORMAL_LPDF, {late, late, 1}, lp);
  g.ops[id].variant = 0x06;
  terms.push_back(lp);

  Graph ref = g;
  reduce_into_result(ref, terms);
  const std::vector<double> want = run_grad(std::move(ref), fills);

  const int n_inplace = make_inplace_updates(g, {});
  // The write feeding `second_in`'s reader must stay a copy; the writes
  // after it are still free to be destructive.
  expect("late reader blocks one write", n_inplace == L - 2);
  reduce_into_result(g, terms);
  const std::vector<double> got = run_grad_twice(std::move(g), fills);
  for (size_t i = 0; i < want.size() && i < got.size(); ++i)
    expect_close(("reader v" + std::to_string(i)).c_str(), got[i], want[i]);
}

// A root (read straight out of the arena, no consuming op) blocks it too.
static void test_bail_root() {
  const int L = 6;
  Fills fills;
  std::vector<int> terms;
  int n_vec = 0;
  Graph g = build_chain(L, fills, terms, &n_vec);
  int second_in = -1, seen = 0;
  for (const Op& op : g.ops)
    if (op.opcode == OP_SET_INDEX && ++seen == 2) second_in = op.in[0];
  const int n_inplace = make_inplace_updates(g, {second_in});
  expect("root blocks one write", n_inplace == L - 2);
}

int main() {
  test_chain_collapses();
  test_bail_later_reader();
  test_bail_root();
  if (failures) { std::printf("%d failures\n", failures); return 1; }
  std::printf("test_inplace OK\n");
  return 0;
}
