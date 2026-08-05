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

void lognormal_fwd(KernelCtx& ctx) {
  density_fwd<3>(ctx, [](const auto&... a) {
    stan::math::lognormal_lpdf<false>(a...);
  });
}
void uniform_fwd(KernelCtx& ctx) {
  density_fwd<3>(ctx, [](const auto&... a) {
    stan::math::uniform_lpdf<false>(a...);
  });
}
void double_exp_fwd(KernelCtx& ctx) {
  density_fwd<3>(ctx, [](const auto&... a) {
    stan::math::double_exponential_lpdf<false>(a...);
  });
}
void exponential_fwd(KernelCtx& ctx) {
  density_fwd<2>(ctx, [](const auto&... a) {
    stan::math::exponential_lpdf<false>(a...);
  });
}
void inv_gamma_fwd(KernelCtx& ctx) {
  density_fwd<3>(ctx, [](const auto&... a) {
    stan::math::inv_gamma_lpdf<false>(a...);
  });
}
void std_normal_fwd(KernelCtx& ctx) {
  density_fwd<1>(ctx, [](const auto&... a) {
    stan::math::std_normal_lpdf<false>(a...);
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

void bernoulli_fwd(KernelCtx& ctx) {
  Eigen::Map<const Eigen::VectorXi> y(ctx.idata,
                                      static_cast<Eigen::Index>(ctx.n_idata));
  density_fwd<1>(ctx, [&](const auto& theta) {
    stan::math::bernoulli_lpmf<false>(y, theta);
  });
}
void poisson_fwd(KernelCtx& ctx) {
  Eigen::Map<const Eigen::VectorXi> n(ctx.idata,
                                      static_cast<Eigen::Index>(ctx.n_idata));
  density_fwd<1>(ctx, [&](const auto& lambda) {
    stan::math::poisson_lpmf<false>(n, lambda);
  });
}
void neg_binomial_2_fwd(KernelCtx& ctx) {
  Eigen::Map<const Eigen::VectorXi> n(ctx.idata,
                                      static_cast<Eigen::Index>(ctx.n_idata));
  density_fwd<2>(ctx, [&](const auto&... a) {
    stan::math::neg_binomial_2_lpmf<false>(n, a...);
  });
}
// Binomials carry two int groups; idata = [len_n, n..., len_N, N...].
void binomial_fwd(KernelCtx& ctx) {
  const int ln = static_cast<int>(ctx.idata[0]);
  Eigen::Map<const Eigen::VectorXi> n(ctx.idata + 1, ln);
  const int lN = static_cast<int>(ctx.idata[1 + ln]);
  Eigen::Map<const Eigen::VectorXi> N(ctx.idata + 2 + ln, lN);
  density_fwd<1>(ctx, [&](const auto& theta) {
    stan::math::binomial_lpmf<false>(n, N, theta);
  });
}
void binomial_logit_fwd(KernelCtx& ctx) {
  const int ln = static_cast<int>(ctx.idata[0]);
  Eigen::Map<const Eigen::VectorXi> n(ctx.idata + 1, ln);
  const int lN = static_cast<int>(ctx.idata[1 + ln]);
  Eigen::Map<const Eigen::VectorXi> N(ctx.idata + 2 + ln, lN);
  density_fwd<1>(ctx, [&](const auto& alpha) {
    stan::math::binomial_logit_lpmf<false>(n, N, alpha);
  });
}
// bernoulli_logit_glm(y | X, alpha, beta): X data matrix (row-major slot),
// idata = [y..., rows, cols]. Edges are (x, alpha, beta); X is arg 0.
void bernoulli_logit_glm_fwd(KernelCtx& ctx) {
  const int64_t rows = ctx.idata[ctx.n_idata - 2];
  const int64_t cols = ctx.idata[ctx.n_idata - 1];
  Eigen::Map<const Eigen::VectorXi> y(ctx.idata, rows);
  using RowMat =
      Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
  Eigen::Map<const RowMat> X(ctx.in[0].data, rows, cols);
  sink s;
  int64_t off = 0;
  for (int k = 0; k < 3; ++k) {
    s.buf[k] = ctx.scratch + off;
    off += ctx.in[k].len;
  }
  active_sink() = &s;
  if (ctx.in[1].len == 1 && ctx.in[2].len > 1) {
    stan::math::bernoulli_logit_glm_lpmf<false>(
        y, X.eval(), rvar(ctx.in[1].data[0]), as_rvar(ctx.in[2]));
  } else {
    active_sink() = nullptr;
    throw std::runtime_error("glm: unsupported shape combo");
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
  register_kernel(OP_BINOMIAL_LPMF,
                  Kernel{binomial_fwd, density_bwd<1>, sum_in_lens});
  register_kernel(OP_BINOMIAL_LOGIT_LPMF,
                  Kernel{binomial_logit_fwd, density_bwd<1>, sum_in_lens});
  register_kernel(OP_BERNOULLI_LOGIT_GLM_LPMF,
                  Kernel{bernoulli_logit_glm_fwd, bernoulli_logit_glm_bwd,
                         sum_in_lens});
}

}  // namespace stanrt
