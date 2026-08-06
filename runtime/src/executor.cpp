#include <stanrt/graph.hpp>
#include <stanrt/optable.hpp>

#include <cassert>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>

namespace stanrt {

static Kernel g_table[OP_COUNT_];

Kernel& kernel(uint16_t opcode) {
  assert(opcode < OP_COUNT_);
  return g_table[opcode];
}

void register_kernel(uint16_t opcode, Kernel k) {
  assert(opcode < OP_COUNT_);
  g_table[opcode] = k;
}

void register_elementwise_kernels();
void register_density_kernels();
void register_legacy_kernels();
void register_matrix_kernels();
void register_ode_kernels();
void register_constrain_kernels();
void register_eltwise_kernels();

static void ensure_registered() {
  static const bool once = [] {
    register_elementwise_kernels();
    register_density_kernels();
    register_legacy_kernels();
  register_matrix_kernels();
  register_ode_kernels();
    register_constrain_kernels();
    register_eltwise_kernels();
    return true;
  }();
  (void)once;
}

Executor::Executor(Graph g) : graph_(std::move(g)) {
  ensure_registered();
  bind_();
}

void Executor::bind_() {
  // Parameters first so the gradient vector is contiguous in declaration
  // order; then everything else.
  int64_t off = 0;
  for (auto& s : graph_.slots) {
    if (s.is_param) {
      s.offset = off;
      off += s.len;
    }
  }
  n_params_ = off;
  for (auto& s : graph_.slots) {
    if (!s.is_param) {
      s.offset = off;
      off += s.len;
    }
  }
  arena_len_ = off;
  values_.assign(arena_len_, 0.0);
  adjoints_.assign(arena_len_, 0.0);

  // A slot carries adjoint if it is a parameter or an op writes it. Slots
  // that are neither are data: kernels see a null adjoint Desc and skip them.
  written_.assign(graph_.slots.size(), 0);
  for (const auto& op : graph_.ops) {
    written_[op.out] = 1;
    if (op.out2 >= 0) written_[op.out2] = 1;
  }

  int64_t scratch = 0;
  for (auto& op : graph_.ops) {
    const Kernel& k = kernel(op.opcode);
    if (k.forward == nullptr)
      throw std::runtime_error("opcode not registered: " +
                               std::to_string(op.opcode));
    op.scratch_off = scratch;
    op.scratch_len =
        k.scratch_size ? k.scratch_size(op, graph_.slots.data()) : 0;
    scratch += op.scratch_len;
  }
  scratch_.assign(scratch, 0.0);
}

KernelCtx Executor::make_ctx_(const Op& op, bool backward) {
  KernelCtx ctx;
  ctx.n_in = op.n_in;
  for (int i = 0; i < op.n_in; ++i) {
    const Slot& s = graph_.slots[op.in[i]];
    ctx.in[i] = Desc{values_.data() + s.offset, s.len};
  }
  const Slot& so = graph_.slots[op.out];
  ctx.out = Desc{values_.data() + so.offset, so.len};
  if (op.out2 >= 0) {
    const Slot& s2 = graph_.slots[op.out2];
    ctx.out2 = Desc{values_.data() + s2.offset, s2.len};
  }
  ctx.variant = op.variant;
  ctx.scratch = scratch_.data() + op.scratch_off;
  ctx.idata = op.idata;
  ctx.udata = op.udata;
  ctx.n_idata = op.n_idata;
  if (backward) {
    for (int i = 0; i < op.n_in; ++i) {
      const int si = op.in[i];
      const Slot& s = graph_.slots[si];
      const bool active = s.is_param || written_[si];
      ctx.in_adj[i] =
          Desc{active ? adjoints_.data() + s.offset : nullptr, s.len};
    }
    if (so.len == 1) ctx.out_adj = adjoints_[so.offset];
    ctx.out_adj_vec = Desc{adjoints_.data() + so.offset, so.len};
    if (op.out2 >= 0) ctx.out2_adj = adjoints_[graph_.slots[op.out2].offset];
  }
  return ctx;
}

void Executor::run_forward_only() {
  for (const auto& op : graph_.ops) {
    KernelCtx ctx = make_ctx_(op, /*backward=*/false);
    kernel(op.opcode).forward(ctx);
  }
}

double Executor::forward() {
  run_forward_only();
  const Slot& r = graph_.slots[graph_.result_slot];
  assert(r.len == 1);
  return values_[r.offset];
}

double Executor::gradient(double* grad_out) {
  ++n_grad_evals_;
  const double v = forward();
  std::memset(adjoints_.data(), 0, sizeof(double) * adjoints_.size());
  adjoints_[graph_.slots[graph_.result_slot].offset] = 1.0;
  for (auto it = graph_.ops.rbegin(); it != graph_.ops.rend(); ++it) {
    const Kernel& k = kernel(it->opcode);
    if (!k.backward) continue;
    KernelCtx ctx = make_ctx_(*it, /*backward=*/true);
    k.backward(ctx);
  }
  std::memcpy(grad_out, adjoints_.data(), sizeof(double) * n_params_);
  return v;
}

}  // namespace stanrt
