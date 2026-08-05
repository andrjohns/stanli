// Native elementwise / structural ops with hand-written vjps.
#include <stanrt/graph.hpp>
#include <stanrt/optable.hpp>

#include <cassert>
#include <cmath>

namespace stanrt {
namespace {

// OP_EXP: scalar out = exp(in). Partial is the output itself; no scratch.
void exp_fwd(KernelCtx& ctx) { ctx.out.data[0] = std::exp(ctx.in[0].data[0]); }
void exp_bwd(KernelCtx& ctx) {
  if (ctx.in_adj[0].data) ctx.in_adj[0].data[0] += ctx.out_adj * ctx.out.data[0];
}

// OP_ADD_N: scalar out = sum of scalar inputs.
void add_n_fwd(KernelCtx& ctx) {
  double acc = 0;
  for (int i = 0; i < ctx.n_in; ++i) {
    assert(ctx.in[i].len == 1);
    acc += ctx.in[i].data[0];
  }
  ctx.out.data[0] = acc;
}
void add_n_bwd(KernelCtx& ctx) {
  for (int i = 0; i < ctx.n_in; ++i)
    if (ctx.in_adj[i].data) ctx.in_adj[i].data[0] += ctx.out_adj;
}

// OP_BCAST_FMA: out[i] = a + b * x[i], a and b scalar.
void fma_fwd(KernelCtx& ctx) {
  const double a = ctx.in[0].data[0], b = ctx.in[1].data[0];
  const Desc& x = ctx.in[2];
  for (int64_t i = 0; i < x.len; ++i) ctx.out.data[i] = a + b * x.data[i];
}
void fma_bwd(KernelCtx& ctx) {
  const double b = ctx.in[1].data[0];
  const Desc& x = ctx.in[2];
  const Desc& dout = ctx.out_adj_vec;
  double sum = 0, dot = 0;
  for (int64_t i = 0; i < dout.len; ++i) {
    sum += dout.data[i];
    dot += dout.data[i] * x.data[i];
    if (ctx.in_adj[2].data) ctx.in_adj[2].data[i] += b * dout.data[i];
  }
  if (ctx.in_adj[0].data) ctx.in_adj[0].data[0] += sum;
  if (ctx.in_adj[1].data) ctx.in_adj[1].data[0] += dot;
}

}  // namespace

// Called from Executor's constructor path; a static registrar object in a
// static library gets dropped by the linker.
void register_elementwise_kernels() {
  register_kernel(OP_EXP, Kernel{exp_fwd, exp_bwd, nullptr});
  register_kernel(OP_ADD_N, Kernel{add_n_fwd, add_n_bwd, nullptr});
  register_kernel(OP_BCAST_FMA, Kernel{fma_fwd, fma_bwd, nullptr});
}

}  // namespace stanrt
