// Structured op graph: the runtime's IR and, during reverse mode, its tape.
// M1 builds graphs programmatically; the stanc3 backend (M2) will emit the
// same structure from MIR.
#ifndef STANRT_GRAPH_HPP
#define STANRT_GRAPH_HPP

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <vector>

namespace stanrt {

// A view of one contiguous buffer. len == 1 means scalar.
struct Desc {
  double* data;
  int64_t len;
};

// A value in the graph. Slots with is_param are the unconstrained parameter
// vector, in declaration order; everything else is data or an intermediate.
struct Slot {
  int64_t offset = 0;  // into the value / adjoint arenas (filled at bind)
  int64_t len = 0;
  bool is_param = false;
};

struct Op {
  uint16_t opcode = 0;
  uint8_t variant = 0;  // density kernels: bits 0..5 per-arg activity
                        // (1 = autodiff), bit 7 = propto
  int out = -1;
  int out2 = -1;  // optional second output (e.g. constrain jacobian term)
  int in[6] = {-1, -1, -1, -1, -1, -1};
  int n_in = 0;
  const int* idata = nullptr;  // integer immediates (outcome counts, dims)
  int64_t n_idata = 0;
  // Opaque per-op payload for kernels that need compile-time structure the
  // integer immediates cannot carry (today: the ODE right-hand side).
  const void* udata = nullptr;
  int64_t scratch_off = 0;  // into the scratch arena (filled at bind)
  int64_t scratch_len = 0;
};

struct Graph {
  std::vector<Slot> slots;
  std::vector<Op> ops;
  std::vector<std::vector<int>> idata_pool;  // owns per-op integer arrays
  // Owns per-op opaque payloads (ODE specs); pointers into this outlive
  // lowering because the graph is moved, never copied element-wise.
  std::vector<std::shared_ptr<void>> udata_pool;
  int result_slot = -1;

  int add_slot(int64_t len, bool is_param) {
    slots.push_back(Slot{0, len, is_param});
    return static_cast<int>(slots.size()) - 1;
  }

  int add_op(uint16_t opcode, std::initializer_list<int> ins, int out,
             std::vector<int> idata = {}) {
    Op op;
    op.opcode = opcode;
    op.out = out;
    op.n_in = 0;
    for (int s : ins) op.in[op.n_in++] = s;
    if (!idata.empty()) {
      idata_pool.push_back(std::move(idata));
      op.idata = idata_pool.back().data();
      op.n_idata = static_cast<int64_t>(idata_pool.back().size());
    }
    ops.push_back(op);
    return static_cast<int>(ops.size()) - 1;
  }
};

// Per-call view handed to kernels. Assembled by the executor; kernels never
// see slots or arenas directly.
struct KernelCtx {
  Desc in[6];
  int n_in = 0;
  Desc out{nullptr, 0};
  uint8_t variant = 0;
  double* scratch = nullptr;
  const int* idata = nullptr;
  int64_t n_idata = 0;
  const void* udata = nullptr;
  Desc out2{nullptr, 0};      // second output value (scalar), if any
  // Backward only. Data inputs get {nullptr, len}: kernels skip them.
  Desc in_adj[6];
  double out_adj = 0;         // scalar-output ops
  Desc out_adj_vec{nullptr, 0};  // vector-output ops
  double out2_adj = 0;        // adjoint of the second output
};

class Executor {
 public:
  explicit Executor(Graph g);

  int64_t n_params() const { return n_params_; }
  // The unconstrained parameter vector: the first n_params() arena entries,
  // in parameter-slot declaration order.
  double* params_data() { return values_.data(); }
  double* param_ptr(int slot) { return values_.data() + graph_.slots[slot].offset; }
  double* value_ptr(int slot) { return values_.data() + graph_.slots[slot].offset; }

  // Forward through all ops; returns value of result_slot (must be scalar).
  double forward();
  // Forward for graphs whose result is not a scalar (tests only).
  void run_forward_only();
  // forward() + reverse sweep. grad_out receives d result / d params in
  // param-slot declaration order. Returns the forward value.
  double gradient(double* grad_out);
  int64_t n_grad_evals() const { return n_grad_evals_; }

 private:
  void bind_();
  KernelCtx make_ctx_(const Op& op, bool backward);

  Graph graph_;
  std::vector<double> values_;
  std::vector<double> adjoints_;
  std::vector<double> scratch_;
  std::vector<char> written_;  // slot carries adjoint (param or op output)
  int64_t n_grad_evals_ = 0;
  int64_t n_params_ = 0;
  int64_t arena_len_ = 0;
};

}  // namespace stanrt

#endif
