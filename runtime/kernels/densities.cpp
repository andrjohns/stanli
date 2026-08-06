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

// Compile-time recursion over per-arg ACTIVITY (Mask bit: 1 = autodiff
// rvar, 0 = plain double) with runtime shape branching inside. Matching the
// data/parameter instantiation CmdStan's generated code uses is what makes
// propto term-dropping and evaluation order line up exactly.
template <int NArgs, unsigned Mask, typename F, typename... Bound>
void bind_args_m(KernelCtx& ctx, F&& f, const Bound&... bound) {
  if constexpr (sizeof...(Bound) == NArgs) {
    f(bound...);
  } else {
    constexpr int i = sizeof...(Bound);
    constexpr bool active = ((Mask >> i) & 1u) != 0;
    if (ctx.in[i].len == 1) {
      if constexpr (active) {
        bind_args_m<NArgs, Mask>(ctx, f, bound..., rvar(ctx.in[i].data[0]));
      } else {
        bind_args_m<NArgs, Mask>(ctx, f, bound..., ctx.in[i].data[0]);
      }
    } else {
      if constexpr (active) {
        bind_args_m<NArgs, Mask>(ctx, f, bound..., as_rvar(ctx.in[i]));
      } else {
        bind_args_m<NArgs, Mask>(
            ctx, f, bound...,
            Eigen::Map<const Eigen::VectorXd>(ctx.in[i].data, ctx.in[i].len));
      }
    }
  }
}

// Runtime mask -> compile-time Mask instantiation.
template <int NArgs, typename F, unsigned M = 0>
void mask_dispatch(unsigned mask, KernelCtx& ctx, F&& f) {
  if (mask == M) {
    bind_args_m<NArgs, M>(ctx, f);
    return;
  }
  if constexpr (M + 1 < (1u << NArgs)) {
    mask_dispatch<NArgs, F, M + 1>(mask, ctx, std::forward<F>(f));
  }
}

template <int NArgs, typename FProp, typename FFull>
void density_fwd_v(KernelCtx& ctx, FProp&& fp, FFull&& ff) {
  sink s;
  int64_t off = 0;
  for (int k = 0; k < NArgs; ++k) {
    s.buf[k] = ctx.scratch + off;
    off += ctx.in[k].len;
  }
  const unsigned mask = ctx.variant == 0
                            ? (1u << NArgs) - 1  // default: all active
                            : (ctx.variant & 0x3fu);
  active_sink() = &s;
  if (ctx.variant & 0x80u) {
    mask_dispatch<NArgs>(mask, ctx, fp);
  } else {
    mask_dispatch<NArgs>(mask, ctx, ff);
  }
  active_sink() = nullptr;
  ctx.out.data[0] = s.value;
}

// Back-compat shim for kernels not yet variant-aware.
template <int NArgs, typename F>
void density_fwd(KernelCtx& ctx, F&& f) {
  sink s;
  int64_t off = 0;
  for (int k = 0; k < NArgs; ++k) {
    s.buf[k] = ctx.scratch + off;
    off += ctx.in[k].len;
  }
  active_sink() = &s;
  bind_args_m<NArgs, (1u << NArgs) - 1>(ctx, f);
  active_sink() = nullptr;
  ctx.out.data[0] = s.value;
}

// Partials for argument k live at scratch[sum of lens of args < k]. A scalar
// argument paired with vector ones holds the already-summed partial.
template <int NArgs>
void density_bwd(KernelCtx& ctx) {
  const unsigned mask = ctx.variant == 0 ? (1u << NArgs) - 1
                                         : (ctx.variant & 0x3fu);
  int64_t off = 0;
  for (int k = 0; k < NArgs; ++k) {
    if (((mask >> k) & 1u) != 0 && ctx.in_adj[k].data != nullptr) {
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
  density_fwd_v<3>(
      ctx, [](const auto&... a) { stan::math::normal_lpdf<true>(a...); },
      [](const auto&... a) { stan::math::normal_lpdf<false>(a...); });
}
void cauchy_fwd(KernelCtx& ctx) {
  density_fwd_v<3>(
      ctx, [](const auto&... a) { stan::math::cauchy_lpdf<true>(a...); },
      [](const auto&... a) { stan::math::cauchy_lpdf<false>(a...); });
}
void student_t_fwd(KernelCtx& ctx) {
  density_fwd_v<4>(
      ctx, [](const auto&... a) { stan::math::student_t_lpdf<true>(a...); },
      [](const auto&... a) { stan::math::student_t_lpdf<false>(a...); });
}
void gamma_fwd(KernelCtx& ctx) {
  density_fwd_v<3>(
      ctx, [](const auto&... a) { stan::math::gamma_lpdf<true>(a...); },
      [](const auto&... a) { stan::math::gamma_lpdf<false>(a...); });
}
void beta_fwd(KernelCtx& ctx) {
  density_fwd_v<3>(
      ctx, [](const auto&... a) { stan::math::beta_lpdf<true>(a...); },
      [](const auto&... a) { stan::math::beta_lpdf<false>(a...); });
}

void weibull_fwd(KernelCtx& ctx) {
  density_fwd_v<3>(
      ctx, [](const auto&... a) { stan::math::weibull_lpdf<true>(a...); },
      [](const auto&... a) { stan::math::weibull_lpdf<false>(a...); });
}
void lognormal_fwd(KernelCtx& ctx) {
  density_fwd_v<3>(
      ctx, [](const auto&... a) { stan::math::lognormal_lpdf<true>(a...); },
      [](const auto&... a) { stan::math::lognormal_lpdf<false>(a...); });
}
void uniform_fwd(KernelCtx& ctx) {
  density_fwd_v<3>(
      ctx, [](const auto&... a) { stan::math::uniform_lpdf<true>(a...); },
      [](const auto&... a) { stan::math::uniform_lpdf<false>(a...); });
}
void double_exp_fwd(KernelCtx& ctx) {
  density_fwd_v<3>(
      ctx, [](const auto&... a) { stan::math::double_exponential_lpdf<true>(a...); },
      [](const auto&... a) { stan::math::double_exponential_lpdf<false>(a...); });
}
void exponential_fwd(KernelCtx& ctx) {
  density_fwd_v<2>(
      ctx, [](const auto&... a) { stan::math::exponential_lpdf<true>(a...); },
      [](const auto&... a) { stan::math::exponential_lpdf<false>(a...); });
}
void inv_gamma_fwd(KernelCtx& ctx) {
  density_fwd_v<3>(
      ctx, [](const auto&... a) { stan::math::inv_gamma_lpdf<true>(a...); },
      [](const auto&... a) { stan::math::inv_gamma_lpdf<false>(a...); });
}
void std_normal_fwd(KernelCtx& ctx) {
  density_fwd_v<1>(
      ctx, [](const auto&... a) { stan::math::std_normal_lpdf<true>(a...); },
      [](const auto&... a) { stan::math::std_normal_lpdf<false>(a...); });
}

// ---- lpmfs: integer outcome from idata, not a propagator edge --------------
void poisson_log_fwd(KernelCtx& ctx) {
  Eigen::Map<const Eigen::VectorXi> n(ctx.idata,
                                      static_cast<Eigen::Index>(ctx.n_idata));
  density_fwd_v<1>(
      ctx, [&](const auto& alpha) { stan::math::poisson_log_lpmf<true>(n, alpha); },
      [&](const auto& alpha) { stan::math::poisson_log_lpmf<false>(n, alpha); });
}
void bernoulli_logit_fwd(KernelCtx& ctx) {
  Eigen::Map<const Eigen::VectorXi> y(ctx.idata,
                                      static_cast<Eigen::Index>(ctx.n_idata));
  density_fwd_v<1>(
      ctx, [&](const auto& alpha) { stan::math::bernoulli_logit_lpmf<true>(y, alpha); },
      [&](const auto& alpha) { stan::math::bernoulli_logit_lpmf<false>(y, alpha); });
}

void bernoulli_fwd(KernelCtx& ctx) {
  Eigen::Map<const Eigen::VectorXi> y(ctx.idata,
                                      static_cast<Eigen::Index>(ctx.n_idata));
  density_fwd_v<1>(
      ctx, [&](const auto& theta) { stan::math::bernoulli_lpmf<true>(y, theta); },
      [&](const auto& theta) { stan::math::bernoulli_lpmf<false>(y, theta); });
}
void poisson_fwd(KernelCtx& ctx) {
  Eigen::Map<const Eigen::VectorXi> n(ctx.idata,
                                      static_cast<Eigen::Index>(ctx.n_idata));
  density_fwd_v<1>(
      ctx, [&](const auto& lambda) { stan::math::poisson_lpmf<true>(n, lambda); },
      [&](const auto& lambda) { stan::math::poisson_lpmf<false>(n, lambda); });
}
void neg_binomial_2_fwd(KernelCtx& ctx) {
  Eigen::Map<const Eigen::VectorXi> n(ctx.idata,
                                      static_cast<Eigen::Index>(ctx.n_idata));
  density_fwd_v<2>(
      ctx, [&](const auto&... a) { stan::math::neg_binomial_2_lpmf<true>(n, a...); },
      [&](const auto&... a) { stan::math::neg_binomial_2_lpmf<false>(n, a...); });
}
// Binomials carry two int groups; idata = [len_n, n..., len_N, N...].
// A length of -1 marks a language-level int scalar (stan-math broadcasts
// scalars; a size-1 vector would be a size error against a longer group).
template <typename F>
void with_int_group(const int* p, F&& f) {
  const int len = static_cast<int>(p[0]);
  if (len == -1)
    f(p[1], p + 2);
  else
    f(Eigen::Map<const Eigen::VectorXi>(p + 1, len), p + 1 + len);
}
void binomial_fwd(KernelCtx& ctx) {
  with_int_group(ctx.idata, [&](const auto& n, const int* rest) {
    with_int_group(rest, [&](const auto& N, const int*) {
      density_fwd_v<1>(
          ctx,
          [&](const auto& theta) { stan::math::binomial_lpmf<true>(n, N, theta); },
          [&](const auto& theta) { stan::math::binomial_lpmf<false>(n, N, theta); });
    });
  });
}
void binomial_logit_fwd(KernelCtx& ctx) {
  with_int_group(ctx.idata, [&](const auto& n, const int* rest) {
    with_int_group(rest, [&](const auto& N, const int*) {
      density_fwd_v<1>(
          ctx,
          [&](const auto& alpha) { stan::math::binomial_logit_lpmf<true>(n, N, alpha); },
          [&](const auto& alpha) { stan::math::binomial_logit_lpmf<false>(n, N, alpha); });
    });
  });
}
// bernoulli_logit_glm(y | X, alpha, beta): X data matrix (row-major slot),
// idata = [y..., rows, cols]. Edges are (x, alpha, beta); X is arg 0.
void bernoulli_logit_glm_fwd(KernelCtx& ctx) {
  const int64_t rows = ctx.idata[ctx.n_idata - 2];
  const int64_t cols = ctx.idata[ctx.n_idata - 1];
  Eigen::Map<const Eigen::VectorXi> y(ctx.idata, rows);
  Eigen::Map<const Eigen::MatrixXd> X(ctx.in[0].data, rows, cols);
  sink s;
  int64_t off = 0;
  for (int k = 0; k < 3; ++k) {
    s.buf[k] = ctx.scratch + off;
    off += ctx.in[k].len;
  }
  active_sink() = &s;
  if (ctx.in[1].len == 1) {
    // beta is a vector regardless of its length; alpha scalar.
    stan::math::bernoulli_logit_glm_lpmf<false>(
        y, X, rvar(ctx.in[1].data[0]), as_rvar(ctx.in[2]));
  } else {
    active_sink() = nullptr;
    throw std::runtime_error("glm: vector alpha unsupported in M2");
  }
  active_sink() = nullptr;
  ctx.out.data[0] = s.value;
}
// Edge order (x, alpha, beta): X data (edge 0 skipped by null adjoint),
// alpha scalar, beta vector.
void bernoulli_logit_glm_bwd(KernelCtx& ctx) { density_bwd<3>(ctx); }

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
  register_kernel(OP_LOGNORMAL_LPDF,
                  Kernel{lognormal_fwd, density_bwd<3>, sum_in_lens});
  register_kernel(OP_UNIFORM_LPDF,
                  Kernel{uniform_fwd, density_bwd<3>, sum_in_lens});
  register_kernel(OP_DOUBLE_EXP_LPDF,
                  Kernel{double_exp_fwd, density_bwd<3>, sum_in_lens});
  register_kernel(OP_EXPONENTIAL_LPDF,
                  Kernel{exponential_fwd, density_bwd<2>, sum_in_lens});
  register_kernel(OP_INV_GAMMA_LPDF,
                  Kernel{inv_gamma_fwd, density_bwd<3>, sum_in_lens});
  register_kernel(OP_STD_NORMAL_LPDF,
                  Kernel{std_normal_fwd, density_bwd<1>, sum_in_lens});
  register_kernel(OP_BERNOULLI_LPMF,
                  Kernel{bernoulli_fwd, density_bwd<1>, sum_in_lens});
  register_kernel(OP_POISSON_LPMF,
                  Kernel{poisson_fwd, density_bwd<1>, sum_in_lens});
  register_kernel(OP_NEG_BINOMIAL_2_LPMF,
                  Kernel{neg_binomial_2_fwd, density_bwd<2>, sum_in_lens});
  register_kernel(OP_WEIBULL_LPDF,
                  Kernel{weibull_fwd, density_bwd<3>, sum_in_lens});
  register_kernel(OP_BINOMIAL_LPMF,
                  Kernel{binomial_fwd, density_bwd<1>, sum_in_lens});
  register_kernel(OP_BINOMIAL_LOGIT_LPMF,
                  Kernel{binomial_logit_fwd, density_bwd<1>, sum_in_lens});
  register_kernel(OP_BERNOULLI_LOGIT_GLM_LPMF,
                  Kernel{bernoulli_logit_glm_fwd, bernoulli_logit_glm_bwd,
                         sum_in_lens});
}

}  // namespace stanrt
