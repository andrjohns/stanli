// Elementwise expression ops for lowered MIR arithmetic. Kernels mirror the
// stan-math REV overloads' forward Eigen expressions and their backward
// accumulation shapes (see constrain.cpp for why: packet math vs libm).
// Shape dispatch is runtime: len==1 broadcasts.
#include <stanrt/graph.hpp>
#include <stanrt/optable.hpp>

#include <stan/math/prim.hpp>

namespace stanrt {
namespace {

using Arr = Eigen::Array<double, -1, 1>;
using MapA = Eigen::Map<Arr>;
using CMapA = Eigen::Map<const Arr>;

inline CMapA in_a(const KernelCtx& ctx, int i) {
  return CMapA(ctx.in[i].data, ctx.in[i].len);
}
inline MapA out_a(KernelCtx& ctx) { return MapA(ctx.out.data, ctx.out.len); }
inline CMapA dout_a(const KernelCtx& ctx) {
  return CMapA(ctx.out_adj_vec.data, ctx.out_adj_vec.len);
}
inline MapA dx_a(KernelCtx& ctx, int i) {
  return MapA(ctx.in_adj[i].data, ctx.in_adj[i].len);
}
inline bool scal(const KernelCtx& ctx, int i) { return ctx.in[i].len == 1; }

// ---- binaries --------------------------------------------------------------
void add_fwd(KernelCtx& ctx) {
  if (scal(ctx, 0) && scal(ctx, 1))
    ctx.out.data[0] = ctx.in[0].data[0] + ctx.in[1].data[0];
  else if (scal(ctx, 1))
    out_a(ctx) = in_a(ctx, 0) + ctx.in[1].data[0];
  else if (scal(ctx, 0))
    out_a(ctx) = ctx.in[0].data[0] + in_a(ctx, 1);
  else
    out_a(ctx) = in_a(ctx, 0) + in_a(ctx, 1);
}
// Scalar-broadcast adjoints accumulate ascending, directly into the
// accumulator: the rev overloads' reverse callbacks loop coefficients in
// ascending order (measured against add(var, Matrix<var>); a local Eigen
// sum added once differs by 1 ULP).
void add_bwd(KernelCtx& ctx) {
  for (int k = 0; k < 2; ++k) {
    if (!ctx.in_adj[k].data) continue;
    if (scal(ctx, k)) {
      if (ctx.out.len == 1) {
        ctx.in_adj[k].data[0] += ctx.out_adj;
      } else {
        for (int64_t i = 0; i < ctx.out.len; ++i)
          ctx.in_adj[k].data[0] += ctx.out_adj_vec.data[i];
      }
    } else {
      dx_a(ctx, k) += dout_a(ctx);
    }
  }
}

void sub_fwd(KernelCtx& ctx) {
  if (scal(ctx, 0) && scal(ctx, 1))
    ctx.out.data[0] = ctx.in[0].data[0] - ctx.in[1].data[0];
  else if (scal(ctx, 1))
    out_a(ctx) = in_a(ctx, 0) - ctx.in[1].data[0];
  else if (scal(ctx, 0))
    out_a(ctx) = ctx.in[0].data[0] - in_a(ctx, 1);
  else
    out_a(ctx) = in_a(ctx, 0) - in_a(ctx, 1);
}
void sub_bwd(KernelCtx& ctx) {
  if (ctx.in_adj[0].data) {
    if (scal(ctx, 0)) {
      if (ctx.out.len == 1) {
        ctx.in_adj[0].data[0] += ctx.out_adj;
      } else {
        for (int64_t i = 0; i < ctx.out.len; ++i)
          ctx.in_adj[0].data[0] += ctx.out_adj_vec.data[i];
      }
    } else {
      dx_a(ctx, 0) += dout_a(ctx);
    }
  }
  if (ctx.in_adj[1].data) {
    if (scal(ctx, 1)) {
      if (ctx.out.len == 1) {
        ctx.in_adj[1].data[0] -= ctx.out_adj;
      } else {
        for (int64_t i = 0; i < ctx.out.len; ++i)
          ctx.in_adj[1].data[0] -= ctx.out_adj_vec.data[i];
      }
    } else {
      dx_a(ctx, 1) -= dout_a(ctx);
    }
  }
}

void mul_fwd(KernelCtx& ctx) {
  if (scal(ctx, 0) && scal(ctx, 1))
    ctx.out.data[0] = ctx.in[0].data[0] * ctx.in[1].data[0];
  else if (scal(ctx, 1))
    out_a(ctx) = in_a(ctx, 0) * ctx.in[1].data[0];
  else if (scal(ctx, 0))
    out_a(ctx) = ctx.in[0].data[0] * in_a(ctx, 1);
  else
    out_a(ctx) = in_a(ctx, 0) * in_a(ctx, 1);  // elt_multiply
}
void mul_bwd(KernelCtx& ctx) {
  const bool s0 = scal(ctx, 0), s1 = scal(ctx, 1);
  if (ctx.out.len == 1) {
    if (ctx.in_adj[0].data)
      ctx.in_adj[0].data[0] += ctx.out_adj * ctx.in[1].data[0];
    if (ctx.in_adj[1].data)
      ctx.in_adj[1].data[0] += ctx.out_adj * ctx.in[0].data[0];
    return;
  }
  if (ctx.in_adj[0].data) {
    if (s0) {
      for (int64_t i = 0; i < ctx.out.len; ++i)
        ctx.in_adj[0].data[0] += ctx.out_adj_vec.data[i] * ctx.in[1].data[i];
    } else if (s1) {
      dx_a(ctx, 0) += dout_a(ctx) * ctx.in[1].data[0];
    } else {
      dx_a(ctx, 0) += dout_a(ctx) * in_a(ctx, 1);
    }
  }
  if (ctx.in_adj[1].data) {
    if (s1) {
      for (int64_t i = 0; i < ctx.out.len; ++i)
        ctx.in_adj[1].data[0] += ctx.out_adj_vec.data[i] * ctx.in[0].data[i];
    } else if (s0) {
      dx_a(ctx, 1) += dout_a(ctx) * ctx.in[0].data[0];
    } else {
      dx_a(ctx, 1) += dout_a(ctx) * in_a(ctx, 0);
    }
  }
}

void div_fwd(KernelCtx& ctx) {
  if (scal(ctx, 0) && scal(ctx, 1))
    ctx.out.data[0] = ctx.in[0].data[0] / ctx.in[1].data[0];
  else if (scal(ctx, 1))
    out_a(ctx) = in_a(ctx, 0) / ctx.in[1].data[0];
  else if (scal(ctx, 0))
    out_a(ctx) = ctx.in[0].data[0] / in_a(ctx, 1);
  else
    out_a(ctx) = in_a(ctx, 0) / in_a(ctx, 1);  // elt_divide
}
void div_bwd(KernelCtx& ctx) {
  const bool s0 = scal(ctx, 0), s1 = scal(ctx, 1);
  if (ctx.out.len == 1) {
    const double b = ctx.in[1].data[0];
    if (ctx.in_adj[0].data) ctx.in_adj[0].data[0] += ctx.out_adj / b;
    if (ctx.in_adj[1].data)
      ctx.in_adj[1].data[0] += -ctx.out_adj * ctx.out.data[0] / b;
    return;
  }
  CMapA out_v(ctx.out.data, ctx.out.len);
  if (!s0 && !s1) {
    // rev elt_divide: ret_div = dout/b; a.adj += ret_div;
    // b.adj -= out * ret_div (same grouping, same reuse).
    Arr ret_div = dout_a(ctx) / in_a(ctx, 1);
    if (ctx.in_adj[0].data) dx_a(ctx, 0) += ret_div;
    if (ctx.in_adj[1].data) dx_a(ctx, 1) -= out_v * ret_div;
    return;
  }
  if (ctx.in_adj[0].data) {
    if (s0) {
      for (int64_t i = 0; i < ctx.out.len; ++i)
        ctx.in_adj[0].data[0] += ctx.out_adj_vec.data[i] / ctx.in[1].data[i];
    } else {
      dx_a(ctx, 0) += dout_a(ctx) / ctx.in[1].data[0];
    }
  }
  if (ctx.in_adj[1].data) {
    if (s1) {
      for (int64_t i = 0; i < ctx.out.len; ++i)
        ctx.in_adj[1].data[0] +=
            -ctx.out_adj_vec.data[i] * ctx.out.data[i] / ctx.in[1].data[0];
    } else {
      dx_a(ctx, 1) += -dout_a(ctx) * out_v / in_a(ctx, 1);
    }
  }
}

void pow_fwd(KernelCtx& ctx) {
  ctx.out.data[0] = std::pow(ctx.in[0].data[0], ctx.in[1].data[0]);
}
void pow_bwd(KernelCtx& ctx) {
  const double a = ctx.in[0].data[0], b = ctx.in[1].data[0];
  const double v = ctx.out.data[0];
  if (ctx.in_adj[0].data)
    ctx.in_adj[0].data[0] += ctx.out_adj * b * v / a;
  if (ctx.in_adj[1].data)
    ctx.in_adj[1].data[0] += ctx.out_adj * std::log(a) * v;
}

void dot_fwd(KernelCtx& ctx) {
  ctx.out.data[0] = (in_a(ctx, 0) * in_a(ctx, 1)).sum();
}
void dot_bwd(KernelCtx& ctx) {
  if (ctx.in_adj[0].data) dx_a(ctx, 0) += ctx.out_adj * in_a(ctx, 1);
  if (ctx.in_adj[1].data) dx_a(ctx, 1) += ctx.out_adj * in_a(ctx, 0);
}

// ---- unaries ---------------------------------------------------------------
// AoS Matrix<var> unaries route through apply_scalar_unary: scalar libm per
// element, NOT Eigen packet math. Transcendental kernels therefore use
// scalar loops; sqrt (IEEE-exact) and neg/square (exact) may vectorize.
void negu_fwd(KernelCtx& ctx) { out_a(ctx) = -in_a(ctx, 0); }
void negu_bwd(KernelCtx& ctx) {
  if (ctx.in_adj[0].data) {
    if (ctx.out.len == 1)
      ctx.in_adj[0].data[0] -= ctx.out_adj;
    else
      dx_a(ctx, 0) -= dout_a(ctx);
  }
}
void expv_fwd(KernelCtx& ctx) {
  for (int64_t i = 0; i < ctx.out.len; ++i)
    ctx.out.data[i] = std::exp(ctx.in[0].data[i]);
}
void expv_bwd(KernelCtx& ctx) {
  if (!ctx.in_adj[0].data) return;
  CMapA out_v(ctx.out.data, ctx.out.len);
  if (ctx.out.len == 1)
    ctx.in_adj[0].data[0] += ctx.out_adj * ctx.out.data[0];
  else
    dx_a(ctx, 0) += dout_a(ctx) * out_v;
}
void logv_fwd(KernelCtx& ctx) {
  for (int64_t i = 0; i < ctx.out.len; ++i)
    ctx.out.data[i] = std::log(ctx.in[0].data[i]);
}
void logv_bwd(KernelCtx& ctx) {
  if (!ctx.in_adj[0].data) return;
  if (ctx.out.len == 1)
    ctx.in_adj[0].data[0] += ctx.out_adj / ctx.in[0].data[0];
  else
    dx_a(ctx, 0) += dout_a(ctx) / in_a(ctx, 0);
}
// Matrix<var> inv_logit resolves to a vectorized overload (packet values);
// scalar var inv_logit uses libm. Match the vectorized path for len > 1.
void invlogit_fwd(KernelCtx& ctx) {
  if (ctx.out.len == 1) {
    ctx.out.data[0] = stan::math::inv_logit(ctx.in[0].data[0]);
  } else {
    out_a(ctx) = stan::math::inv_logit(in_a(ctx, 0).matrix().eval().array());
  }
}
void invlogit_bwd(KernelCtx& ctx) {
  if (!ctx.in_adj[0].data) return;
  CMapA out_v(ctx.out.data, ctx.out.len);
  if (ctx.out.len == 1)
    ctx.in_adj[0].data[0] +=
        ctx.out_adj * ctx.out.data[0] * (1.0 - ctx.out.data[0]);
  else
    dx_a(ctx, 0) += dout_a(ctx) * out_v * (1.0 - out_v);
}
void sqrtv_fwd(KernelCtx& ctx) { out_a(ctx) = in_a(ctx, 0).sqrt(); }
void sqrtv_bwd(KernelCtx& ctx) {
  if (!ctx.in_adj[0].data) return;
  CMapA out_v(ctx.out.data, ctx.out.len);
  if (ctx.out.len == 1)
    ctx.in_adj[0].data[0] += ctx.out_adj / (2.0 * ctx.out.data[0]);
  else
    dx_a(ctx, 0) += dout_a(ctx) / (2.0 * out_v);
}
void squarev_fwd(KernelCtx& ctx) { out_a(ctx) = in_a(ctx, 0).square(); }
void squarev_bwd(KernelCtx& ctx) {
  if (!ctx.in_adj[0].data) return;
  if (ctx.out.len == 1)
    ctx.in_adj[0].data[0] += ctx.out_adj * 2.0 * ctx.in[0].data[0];
  else
    dx_a(ctx, 0) += dout_a(ctx) * 2.0 * in_a(ctx, 0);
}
void log1mv_fwd(KernelCtx& ctx) {
  if (ctx.out.len == 1) {
    ctx.out.data[0] = stan::math::log1m(ctx.in[0].data[0]);
  } else {
    out_a(ctx) = stan::math::log1m(in_a(ctx, 0).matrix().eval().array());
  }
}
void log1mv_bwd(KernelCtx& ctx) {
  if (!ctx.in_adj[0].data) return;
  if (ctx.out.len == 1)
    ctx.in_adj[0].data[0] += ctx.out_adj / (ctx.in[0].data[0] - 1.0);
  else
    dx_a(ctx, 0) += dout_a(ctx) / (in_a(ctx, 0) - 1.0);
}

}  // namespace

void register_eltwise_kernels() {
  register_kernel(OP_ADD, Kernel{add_fwd, add_bwd, nullptr});
  register_kernel(OP_SUB, Kernel{sub_fwd, sub_bwd, nullptr});
  register_kernel(OP_MUL, Kernel{mul_fwd, mul_bwd, nullptr});
  register_kernel(OP_DIV, Kernel{div_fwd, div_bwd, nullptr});
  register_kernel(OP_POW, Kernel{pow_fwd, pow_bwd, nullptr});
  register_kernel(OP_DOT, Kernel{dot_fwd, dot_bwd, nullptr});
  register_kernel(OP_NEG, Kernel{negu_fwd, negu_bwd, nullptr});
  register_kernel(OP_EXPV, Kernel{expv_fwd, expv_bwd, nullptr});
  register_kernel(OP_LOGV, Kernel{logv_fwd, logv_bwd, nullptr});
  register_kernel(OP_INV_LOGIT, Kernel{invlogit_fwd, invlogit_bwd, nullptr});
  register_kernel(OP_SQRT, Kernel{sqrtv_fwd, sqrtv_bwd, nullptr});
  register_kernel(OP_SQUARE, Kernel{squarev_fwd, squarev_bwd, nullptr});
  register_kernel(OP_LOG1M, Kernel{log1mv_fwd, log1mv_bwd, nullptr});
}

}  // namespace stanrt
