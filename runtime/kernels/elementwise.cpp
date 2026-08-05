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

// OP_SLICE_STRIDED: out[i] = in[start + i*stride], idata = {start, stride}.
// Row extraction from column-major data matrices.
void slice_strided_fwd(KernelCtx& ctx) {
  const int64_t start = ctx.idata[0], stride = ctx.idata[1];
  for (int64_t i = 0; i < ctx.out.len; ++i)
    ctx.out.data[i] = ctx.in[0].data[start + i * stride];
}
void slice_strided_bwd(KernelCtx& ctx) {
  if (!ctx.in_adj[0].data) return;
  const int64_t start = ctx.idata[0], stride = ctx.idata[1];
  for (int64_t i = 0; i < ctx.out.len; ++i)
    ctx.in_adj[0].data[start + i * stride] += ctx.out_adj_vec.data[i];
}

// OP_GATHER: out[k] = in[idata[k]] (0-based indices); adjoints scatter-add
// in ascending k, matching the var path's edge order (duplicates included).
void gather_fwd(KernelCtx& ctx) {
  for (int64_t k = 0; k < ctx.out.len; ++k)
    ctx.out.data[k] = ctx.in[0].data[ctx.idata[k]];
}
void gather_bwd(KernelCtx& ctx) {
  if (!ctx.in_adj[0].data) return;
  for (int64_t k = 0; k < ctx.out.len; ++k)
    ctx.in_adj[0].data[ctx.idata[k]] += ctx.out_adj_vec.data[k];
}

// OP_CONCAT2: out = [in[0]; in[1]] (contiguous; serves append_row of
// vectors and append_col of col-major matrices). Backward splits.
void concat2_fwd(KernelCtx& ctx) {
  for (int64_t i = 0; i < ctx.in[0].len; ++i)
    ctx.out.data[i] = ctx.in[0].data[i];
  for (int64_t i = 0; i < ctx.in[1].len; ++i)
    ctx.out.data[ctx.in[0].len + i] = ctx.in[1].data[i];
}
void concat2_bwd(KernelCtx& ctx) {
  if (ctx.in_adj[0].data)
    for (int64_t i = 0; i < ctx.in[0].len; ++i)
      ctx.in_adj[0].data[i] += ctx.out_adj_vec.data[i];
  if (ctx.in_adj[1].data)
    for (int64_t i = 0; i < ctx.in[1].len; ++i)
      ctx.in_adj[1].data[i] += ctx.out_adj_vec.data[ctx.in[0].len + i];
}

// OP_REP_MAT: idata = {R, C, mode}. mode 0: scalar fill; mode 1: vector
// replicated as C columns (out[j*R+i] = v[i]); mode 2: row_vector across R
// rows (out[j*R+i] = v[j]). AoS rep_matrix copies var handles, so each
// source vari accumulates its consumers' adjoints in flat col-major order;
// the backward loops preserve exactly that per-vari sequence.
void rep_mat_fwd(KernelCtx& ctx) {
  const int64_t R = ctx.idata[0], C = ctx.idata[1], mode = ctx.idata[2];
  const double* v = ctx.in[0].data;
  for (int64_t j = 0; j < C; ++j)
    for (int64_t i = 0; i < R; ++i)
      ctx.out.data[j * R + i] = mode == 0 ? v[0] : mode == 1 ? v[i] : v[j];
}
void rep_mat_bwd(KernelCtx& ctx) {
  if (!ctx.in_adj[0].data) return;
  const int64_t R = ctx.idata[0], C = ctx.idata[1], mode = ctx.idata[2];
  const double* dout = ctx.out_adj_vec.data;
  double* dv = ctx.in_adj[0].data;
  for (int64_t j = 0; j < C; ++j)
    for (int64_t i = 0; i < R; ++i)
      dv[mode == 0 ? 0 : mode == 1 ? i : j] += dout[j * R + i];
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
  register_kernel(OP_SLICE_STRIDED,
                  Kernel{slice_strided_fwd, slice_strided_bwd, nullptr});
  register_kernel(OP_GATHER, Kernel{gather_fwd, gather_bwd, nullptr});
  register_kernel(OP_CONCAT2, Kernel{concat2_fwd, concat2_bwd, nullptr});
  register_kernel(OP_REP_MAT, Kernel{rep_mat_fwd, rep_mat_bwd, nullptr});
}

}  // namespace stanrt
