// Recorder density kernels: unmodified stan-math prim/prob templates
// instantiated on rvar, depositing value + partials via the active sink.
//
// One kernel per signature. Every real argument is bound as rvar (zero-copy
// for vectors), so all partials are computed and the data/parameter
// distinction is runtime-only: backward contracts an argument's partials
// only if the executor gave it an adjoint buffer. Shape (scalar vs vector)
// is the one compile-time axis; bind_args expands the 2^N combinations and
// the argument lengths select one per call.
#include <stanrt/graph.hpp>
#include <stanrt/optable.hpp>
#include <stanrt/recorder.hpp>

// The full prim aggregate, not per-density headers: densities call helpers
// like square() through two-phase lookup and rely on the whole overload set
// being visible at instantiation, exactly as today's generated C++ does.
#include <stan/math/prim.hpp>

namespace stanrt {
namespace {

template <int NArgs, typename F, typename... Bound>
void bind_args(KernelCtx& ctx, F&& f, const Bound&... bound) {
  if constexpr (sizeof...(Bound) == NArgs) {
    f(bound...);
  } else {
    constexpr int i = sizeof...(Bound);
    if (ctx.in[i].len == 1) {
      bind_args<NArgs>(ctx, f, bound..., rvar(ctx.in[i].data[0]));
    } else {
      bind_args<NArgs>(ctx, f, bound..., as_rvar(ctx.in[i]));
    }
  }
}

template <int NArgs, typename F>
void density_fwd(KernelCtx& ctx, F&& f) {
  sink s;
  int64_t off = 0;
  for (int k = 0; k < NArgs; ++k) {
    s.buf[k] = ctx.scratch + off;
    off += ctx.in[k].len;
  }
  active_sink() = &s;
  bind_args<NArgs>(ctx, f);
  active_sink() = nullptr;
  ctx.out.data[0] = s.value;
}

// Partials for argument k live at scratch[sum of lens of args < k]. A scalar
// argument paired with vector ones holds the already-summed partial.
template <int NArgs>
void density_bwd(KernelCtx& ctx) {
  int64_t off = 0;
  for (int k = 0; k < NArgs; ++k) {
    if (ctx.in_adj[k].data != nullptr) {
      for (int64_t i = 0; i < ctx.in[k].len; ++i)
        ctx.in_adj[k].data[i] += ctx.out_adj * ctx.scratch[off + i];
    }
    off += ctx.in[k].len;
  }
}

int64_t sum_in_lens(const Op& op, const Slot* slots) {
  int64_t t = 0;
  for (int i = 0; i < op.n_in; ++i) t += slots[op.in[i]].len;
  return t;
}

// ---- lpdfs: real args only -------------------------------------------------
void normal_fwd(KernelCtx& ctx) {
  density_fwd<3>(ctx, [](const auto&... a) {
    stan::math::normal_lpdf<false>(a...);
  });
}
void cauchy_fwd(KernelCtx& ctx) {
  density_fwd<3>(ctx, [](const auto&... a) {
    stan::math::cauchy_lpdf<false>(a...);
  });
}
void student_t_fwd(KernelCtx& ctx) {
  density_fwd<4>(ctx, [](const auto&... a) {
    stan::math::student_t_lpdf<false>(a...);
  });
}
void gamma_fwd(KernelCtx& ctx) {
  density_fwd<3>(ctx, [](const auto&... a) {
    stan::math::gamma_lpdf<false>(a...);
  });
}
void beta_fwd(KernelCtx& ctx) {
  density_fwd<3>(ctx, [](const auto&... a) {
    stan::math::beta_lpdf<false>(a...);
  });
}

// ---- lpmfs: integer outcome from idata, not a propagator edge --------------
void poisson_log_fwd(KernelCtx& ctx) {
  Eigen::Map<const Eigen::VectorXi> n(ctx.idata,
                                      static_cast<Eigen::Index>(ctx.n_idata));
  density_fwd<1>(ctx, [&](const auto& alpha) {
    stan::math::poisson_log_lpmf<false>(n, alpha);
  });
}
void bernoulli_logit_fwd(KernelCtx& ctx) {
  Eigen::Map<const Eigen::VectorXi> y(ctx.idata,
                                      static_cast<Eigen::Index>(ctx.n_idata));
  density_fwd<1>(ctx, [&](const auto& alpha) {
    stan::math::bernoulli_logit_lpmf<false>(y, alpha);
  });
}

}  // namespace

void register_density_kernels() {
  register_kernel(OP_NORMAL_LPDF,
                  Kernel{normal_fwd, density_bwd<3>, sum_in_lens});
  register_kernel(OP_CAUCHY_LPDF,
                  Kernel{cauchy_fwd, density_bwd<3>, sum_in_lens});
  register_kernel(OP_STUDENT_T_LPDF,
                  Kernel{student_t_fwd, density_bwd<4>, sum_in_lens});
  register_kernel(OP_GAMMA_LPDF,
                  Kernel{gamma_fwd, density_bwd<3>, sum_in_lens});
  register_kernel(OP_BETA_LPDF,
                  Kernel{beta_fwd, density_bwd<3>, sum_in_lens});
  register_kernel(OP_POISSON_LOG_LPMF,
                  Kernel{poisson_log_fwd, density_bwd<1>, sum_in_lens});
  register_kernel(OP_BERNOULLI_LOGIT_LPMF,
                  Kernel{bernoulli_logit_fwd, density_bwd<1>, sum_in_lens});
}

}  // namespace stanrt
