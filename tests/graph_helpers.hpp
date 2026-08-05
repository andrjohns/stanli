// Shared helper: build and run a one-op graph, returning value + gradient.
#ifndef STANRT_TESTS_GRAPH_HELPERS_HPP
#define STANRT_TESTS_GRAPH_HELPERS_HPP

#include <stanrt/graph.hpp>
#include <stanrt/optable.hpp>

#include <vector>

namespace stanrt {
namespace testutil {

struct RunResult {
  double value;
  std::vector<double> grad;
};

// One op, scalar output at result_slot. vals[i]/params[i] describe inputs.
inline RunResult run_one_op(uint16_t opcode,
                            const std::vector<std::vector<double>>& vals,
                            const std::vector<bool>& params,
                            std::vector<int> idata = {}) {
  Graph g;
  std::vector<int> slots;
  int64_t n_par = 0;
  for (size_t i = 0; i < vals.size(); ++i) {
    slots.push_back(g.add_slot((int64_t)vals[i].size(), params[i]));
    if (params[i]) n_par += (int64_t)vals[i].size();
  }
  const int lp = g.add_slot(1, false);
  Op op;
  op.opcode = opcode;
  op.out = lp;
  op.n_in = 0;
  for (int s : slots) op.in[op.n_in++] = s;
  if (!idata.empty()) {
    g.idata_pool.push_back(std::move(idata));
    op.idata = g.idata_pool.back().data();
    op.n_idata = (int64_t)g.idata_pool.back().size();
  }
  g.ops.push_back(op);
  g.result_slot = lp;

  Executor ex(std::move(g));
  for (size_t i = 0; i < vals.size(); ++i) {
    double* p = ex.value_ptr(slots[i]);
    for (size_t j = 0; j < vals[i].size(); ++j) p[j] = vals[i][j];
  }
  RunResult r;
  r.grad.assign(n_par, 0.0);
  r.value = ex.gradient(r.grad.data());
  return r;
}

// One elementwise op feeding OP_SUM_VEC so vector outputs reduce to a scalar
// lp (sum), matching a var reference of sum(f(args)).
inline RunResult run_op_sum(uint16_t opcode, int64_t out_len,
                            const std::vector<std::vector<double>>& vals,
                            const std::vector<bool>& params) {
  Graph g;
  std::vector<int> slots;
  int64_t n_par = 0;
  for (size_t i = 0; i < vals.size(); ++i) {
    slots.push_back(g.add_slot((int64_t)vals[i].size(), params[i]));
    if (params[i]) n_par += (int64_t)vals[i].size();
  }
  const int out = g.add_slot(out_len, false);
  const int lp = g.add_slot(1, false);
  Op op;
  op.opcode = opcode;
  op.out = out;
  op.n_in = 0;
  for (int s : slots) op.in[op.n_in++] = s;
  g.ops.push_back(op);
  if (out_len == 1) {
    g.add_op(OP_ADD_N, {out}, lp);
  } else {
    g.add_op(OP_SUM_VEC, {out}, lp);
  }
  g.result_slot = lp;

  Executor ex(std::move(g));
  for (size_t i = 0; i < vals.size(); ++i) {
    double* p = ex.value_ptr(slots[i]);
    for (size_t j = 0; j < vals[i].size(); ++j) p[j] = vals[i][j];
  }
  RunResult r;
  r.grad.assign(n_par, 0.0);
  r.value = ex.gradient(r.grad.data());
  return r;
}

}  // namespace testutil
}  // namespace stanrt

#endif
