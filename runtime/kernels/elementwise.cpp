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
  // Element order descending with direct accumulation, matching the var
  // tape's reverse replay of the per-element vari chain: local ascending
  // partial sums differ from it by 1 ULP.
  for (int64_t i = dout.len - 1; i >= 0; --i) {
    if (ctx.in_adj[0].data) ctx.in_adj[0].data[0] += dout.data[i];
    if (ctx.in_adj[1].data) ctx.in_adj[1].data[0] += dout.data[i] * x.data[i];
    if (ctx.in_adj[2].data) ctx.in_adj[2].data[i] += b * dout.data[i];
  }
}

// OP_MATVEC: out = X * beta, X data laid out COLUMN-major (Stan/Eigen
// convention), idata = {rows, cols}. X as a parameter is out of scope.
void matvec_fwd(KernelCtx& ctx) {
  const int64_t rows = ctx.idata[0], cols = ctx.idata[1];
  const double* X = ctx.in[0].data;
  const double* b = ctx.in[1].data;
  for (int64_t r = 0; r < rows; ++r) {
    double acc = 0;
    for (int64_t c = 0; c < cols; ++c) acc += X[c * rows + r] * b[c];
    ctx.out.data[r] = acc;
  }
}
void matvec_bwd(KernelCtx& ctx) {
  const int64_t rows = ctx.idata[0], cols = ctx.idata[1];
  const double* X = ctx.in[0].data;
  const double* dout = ctx.out_adj_vec.data;
  if (ctx.in_adj[1].data != nullptr) {
    // Rows descending: the var tape replays eta's entries in reverse
    // creation order, and matching its accumulation order keeps parity
    // with the reference bitwise.
    for (int64_t r = rows - 1; r >= 0; --r)
      for (int64_t c = 0; c < cols; ++c)
        ctx.in_adj[1].data[c] += X[c * rows + r] * dout[r];
  }
}

// OP_SUM_VEC: scalar out = sum(x), ascending like Eigen's redux.
void sum_vec_fwd(KernelCtx& ctx) {
  double acc = 0;
  for (int64_t i = 0; i < ctx.in[0].len; ++i) acc += ctx.in[0].data[i];
  ctx.out.data[0] = acc;
}
void sum_vec_bwd(KernelCtx& ctx) {
  if (ctx.in_adj[0].data)
    for (int64_t i = 0; i < ctx.in[0].len; ++i)
      ctx.in_adj[0].data[i] += ctx.out_adj;
}

// OP_INDEX: scalar out = in[flat], idata = {flat}. Backward scatters.
void index_fwd(KernelCtx& ctx) {
  ctx.out.data[0] = ctx.in[0].data[ctx.idata[0]];
}
void index_bwd(KernelCtx& ctx) {
  if (ctx.in_adj[0].data) ctx.in_adj[0].data[ctx.idata[0]] += ctx.out_adj;
}

// OP_SET_INDEX: out = copy(in[0]) with out[flat] = in[1] (scalar).
void set_index_fwd(KernelCtx& ctx) {
  for (int64_t i = 0; i < ctx.out.len; ++i) ctx.out.data[i] = ctx.in[0].data[i];
  ctx.out.data[ctx.idata[0]] = ctx.in[1].data[0];
}
void set_index_bwd(KernelCtx& ctx) {
  const int64_t f = ctx.idata[0];
  if (ctx.in_adj[0].data)
    for (int64_t i = 0; i < ctx.out.len; ++i)
      if (i != f) ctx.in_adj[0].data[i] += ctx.out_adj_vec.data[i];
  if (ctx.in_adj[1].data) ctx.in_adj[1].data[0] += ctx.out_adj_vec.data[f];
}

// OP_SLICE: out = in[start .. start+out.len), idata = {start}.
void slice_fwd(KernelCtx& ctx) {
  const int64_t start = ctx.idata[0];
  for (int64_t i = 0; i < ctx.out.len; ++i)
    ctx.out.data[i] = ctx.in[0].data[start + i];
}
void slice_bwd(KernelCtx& ctx) {
  if (!ctx.in_adj[0].data) return;
  const int64_t start = ctx.idata[0];
  for (int64_t i = 0; i < ctx.out.len; ++i)
    ctx.in_adj[0].data[start + i] += ctx.out_adj_vec.data[i];
}

// OP_SET_SLICE: out = copy(in[0]) with out[start..start+in[1].len) = in[1].
void set_slice_fwd(KernelCtx& ctx) {
  const int64_t start = ctx.idata[0];
  for (int64_t i = 0; i < ctx.out.len; ++i) ctx.out.data[i] = ctx.in[0].data[i];
  for (int64_t i = 0; i < ctx.in[1].len; ++i)
    ctx.out.data[start + i] = ctx.in[1].data[i];
}
void set_slice_bwd(KernelCtx& ctx) {
  const int64_t start = ctx.idata[0], len = ctx.in[1].len;
  if (ctx.in_adj[0].data)
    for (int64_t i = 0; i < ctx.out.len; ++i)
      if (i < start || i >= start + len)
        ctx.in_adj[0].data[i] += ctx.out_adj_vec.data[i];
  if (ctx.in_adj[1].data)
    for (int64_t i = 0; i < len; ++i)
      ctx.in_adj[1].data[i] += ctx.out_adj_vec.data[start + i];
}

}  // namespace

// Called from Executor's constructor path; a static registrar object in a
// static library dropped by the linker.
void register_elementwise_kernels() {
  register_kernel(OP_EXP, Kernel{exp_fwd, exp_bwd, nullptr});
  register_kernel(OP_ADD_N, Kernel{add_n_fwd, add_n_bwd, nullptr});
  register_kernel(OP_BCAST_FMA, Kernel{fma_fwd, fma_bwd, nullptr});
  register_kernel(OP_MATVEC, Kernel{matvec_fwd, matvec_bwd, nullptr});
  register_kernel(OP_SUM_VEC, Kernel{sum_vec_fwd, sum_vec_bwd, nullptr});
  register_kernel(OP_INDEX, Kernel{index_fwd, index_bwd, nullptr});
  register_kernel(OP_SET_INDEX, Kernel{set_index_fwd, set_index_bwd, nullptr});
  register_kernel(OP_SLICE, Kernel{slice_fwd, slice_bwd, nullptr});
  register_kernel(OP_SET_SLICE, Kernel{set_slice_fwd, set_slice_bwd, nullptr});
}

}  // namespace stanrt
