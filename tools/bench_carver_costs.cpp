// Recalibration check for the island cost estimate's constants
// (kValueRegWeight, kOpCost, and graph_op_cost's softmax/log_sum_exp
// multipliers, all in island.cpp). Those constants say a graph op costs
// kOpCost against one island instruction, and a value crossing an island
// boundary costs kValueRegWeight per live-in element and (kOpCost + 3) per
// live-out element. This binary times the same three things on the running
// machine and prints the ratios next to the constants; it changes nothing.
//
// usage: bench_carver_costs [chain_len] [rounds]
#include <stanli/graph.hpp>
#include <stanli/island.hpp>
#include <stanli/optable.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <utility>
#include <vector>

namespace {

using stanli::Executor;
using stanli::Graph;
using Fills = std::vector<std::pair<int, std::vector<double>>>;
using Clock = std::chrono::steady_clock;

// A chain of n scalar OP_ADD ops (p, then t = t + c[k] for k in [0, n)),
// optionally folding in one live_in_width-element vector live-in (summed
// element by element into the chain, so every element is actually read)
// and packing live_out_width elements of running state into one vector
// live-out via SET_INDEX before the chain continues. Varying a width
// changes only how many elements cross the region's boundary, not how
// many distinct live-in slots the compiler has to track, so it stays
// under the carver's own live-in slot limit regardless of width.
Graph build_chain(int n, int live_in_width, int live_out_width,
                  Fills* fills) {
  Graph g;
  const int p = g.add_slot(1, true);
  std::vector<int> c(n);
  for (int k = 0; k < n; ++k) {
    c[k] = g.add_slot(1, false);
    fills->emplace_back(c[k], std::vector<double>{0.1 + 0.001 * k});
  }
  int t = p;
  int wide_out = -1;
  if (live_out_width > 0) {
    const int tmpl = g.add_slot(live_out_width, false);
    fills->emplace_back(tmpl, std::vector<double>((size_t)live_out_width,
                                                  0.0));
    wide_out = tmpl;
  }
  const int half = n / 2;
  for (int k = 0; k < n; ++k) {
    const int nt = g.add_slot(1, false);
    g.add_op(stanli::OP_ADD, {t, c[k]}, nt);
    t = nt;
    if (live_out_width > 0 && k < live_out_width) {
      const int nw = g.add_slot(live_out_width, false);
      g.add_op(stanli::OP_SET_INDEX, {wide_out, t}, nw, {k});
      wide_out = nw;
    }
    if (k == half) break;
  }
  int wide_in = -1;
  if (live_in_width > 0) wide_in = g.add_slot(live_in_width, true);
  for (int k = half + 1; k < n; ++k) {
    const int nt = g.add_slot(1, false);
    g.add_op(stanli::OP_ADD, {t, c[k]}, nt);
    t = nt;
    if (live_in_width > 0) {
      const int idx = g.add_slot(1, false);
      g.add_op(stanli::OP_INDEX, {wide_in}, idx, {(k - half - 1) %
                                                  live_in_width});
      const int nt2 = g.add_slot(1, false);
      g.add_op(stanli::OP_ADD, {t, idx}, nt2);
      t = nt2;
    }
  }
  const int lp = g.add_slot(1, false);
  if (wide_out >= 0) {
    const int reduced = g.add_slot(1, false);
    g.add_op(stanli::OP_LOG_SUM_EXP, {wide_out}, reduced);
    const int nt = g.add_slot(1, false);
    g.add_op(stanli::OP_ADD, {t, reduced}, nt);
    t = nt;
  }
  g.add_op(stanli::OP_MUL, {t, t}, lp);
  g.result_slot = lp;
  return g;
}

double median(std::vector<double>* v) {
  std::sort(v->begin(), v->end());
  return (*v)[v->size() / 2];
}

// Median per-gradient time in nanoseconds, over `rounds` batches of enough
// repeats to clear the clock's resolution.
double time_gradient_ns(Graph g, int rounds) {
  Executor ex(std::move(g));
  std::vector<double> grad((size_t)ex.n_params());
  for (int64_t i = 0; i < ex.n_params(); ++i) ex.params_data()[i] = 0.3;
  ex.gradient(grad.data());  // warm up: first call pays one-time setup
  std::vector<double> per_round;
  for (int r = 0; r < rounds; ++r) {
    int batch = 1;
    double elapsed = 0;
    for (;;) {
      const auto t0 = Clock::now();
      for (int i = 0; i < batch; ++i) ex.gradient(grad.data());
      elapsed = std::chrono::duration<double, std::nano>(Clock::now() - t0)
                    .count();
      if (elapsed > 100000.0) break;
      batch *= 2;
    }
    per_round.push_back(elapsed / (double)batch);
  }
  return median(&per_round);
}

// build_chain's result_slot is the last slot it adds and nothing reads it
// again, so it needs to be pinned as a root or the carver sees a region
// with no live-out worth extracting and refuses it outright.
int carve_always(Graph* g, const Fills& fills) {
  const int result = (int)g->slots.size() - 1;
  setenv("STANLI_ISLAND_ALWAYS", "1", 1);
  const int carved = stanli::carve_islands(*g, fills, {}, {result});
  unsetenv("STANLI_ISLAND_ALWAYS");
  return carved;
}

}  // namespace

int main(int argc, char** argv) {
  const int chain_len = argc > 1 ? std::atoi(argv[1]) : 4000;
  const int rounds = argc > 2 ? std::atoi(argv[2]) : 9;

  // One graph-op dispatch: the same chain, never carved.
  Fills graph_fills;
  Graph graph_g = build_chain(chain_len, 0, 0, &graph_fills);
  const double graph_ns =
      time_gradient_ns(std::move(graph_g), rounds) / (double)chain_len;

  // One island instruction: the same chain forced into a single island.
  Fills island_fills;
  Graph island_g = build_chain(chain_len, 0, 0, &island_fills);
  const int carved = carve_always(&island_g, island_fills);
  if (carved != 1) {
    std::fprintf(stderr,
                 "bench_carver_costs: expected one forced island, got %d\n",
                 carved);
    return 1;
  }
  const double island_ns =
      time_gradient_ns(std::move(island_g), rounds) / (double)chain_len;

  // One boundary crossing per live-in element: two forced islands with
  // the same body, differing only in the width of one live-in vector,
  // divided by that difference. The body is short relative to the width
  // change so the boundary cost is a visible fraction of the total.
  const int body = 64;
  const int narrow_in = 4, wide_in = 2000;
  Fills narrow_in_fills;
  Graph narrow_in_g = build_chain(body, narrow_in, 0, &narrow_in_fills);
  carve_always(&narrow_in_g, narrow_in_fills);
  const double narrow_in_ns = time_gradient_ns(std::move(narrow_in_g), rounds);
  Fills wide_in_fills;
  Graph wide_in_g = build_chain(body, wide_in, 0, &wide_in_fills);
  carve_always(&wide_in_g, wide_in_fills);
  const double wide_in_ns = time_gradient_ns(std::move(wide_in_g), rounds);
  const double live_in_ns =
      (wide_in_ns - narrow_in_ns) / (double)(wide_in - narrow_in);

  // The same for one live-out element.
  const int narrow_out = 4, wide_out = 2000;
  Fills narrow_out_fills;
  Graph narrow_out_g = build_chain(body, 0, narrow_out, &narrow_out_fills);
  carve_always(&narrow_out_g, narrow_out_fills);
  const double narrow_out_ns =
      time_gradient_ns(std::move(narrow_out_g), rounds);
  Fills wide_out_fills;
  Graph wide_out_g = build_chain(body, 0, wide_out, &wide_out_fills);
  carve_always(&wide_out_g, wide_out_fills);
  const double wide_out_ns = time_gradient_ns(std::move(wide_out_g), rounds);
  const double live_out_ns =
      (wide_out_ns - narrow_out_ns) / (double)(wide_out - narrow_out);

  std::printf("chain_len=%d rounds=%d\n", chain_len, rounds);
  std::printf("%-32s %10.3f ns\n", "one graph-op dispatch", graph_ns);
  std::printf("%-32s %10.3f ns\n", "one island instruction", island_ns);
  std::printf("%-32s %10.3f  (kOpCost assumes 5)\n", "graph/island ratio",
             graph_ns / island_ns);
  std::printf("%-32s %10.3f ns\n", "boundary per live-in element",
             live_in_ns);
  std::printf("%-32s %10.3f  (kValueRegWeight*2 assumes 4)\n",
             "live-in / island-instr ratio", live_in_ns / island_ns);
  std::printf("%-32s %10.3f ns\n", "boundary per live-out element",
             live_out_ns);
  std::printf("%-32s %10.3f  ((kOpCost + 3) assumes 8)\n",
             "live-out / island-instr ratio", live_out_ns / island_ns);
  return 0;
}
