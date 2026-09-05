// The GLM kernels. They carry a data matrix and bind their arguments
// explicitly rather than through mask_dispatch; together they were the
// largest single block in densities_common.cpp.
//
// One of the density shards: see densities_impl.hpp for why they
// are split and what they share.
#include "densities_impl.hpp"

namespace stanli {
namespace dens {

// bernoulli_logit_glm(y | X, alpha, beta): X is a data matrix, mapped
// column-major like every other matrix slot.
// idata = [y..., rows, cols]. Edges are (x, alpha, beta); X is arg 0.
void bernoulli_logit_glm_fwd(KernelCtx& ctx) {
  const int64_t rows = ctx.idata[ctx.n_idata - 2];
  const int64_t cols = ctx.idata[ctx.n_idata - 1];
  Eigen::Map<const Eigen::VectorXi> y(ctx.idata, rows);
  Eigen::Map<const Eigen::MatrixXd> X(ctx.in[0].data, rows, cols);
  sink s = sink_for_args(ctx, 3);
  sink_scope active(s);
  // beta is a vector regardless of its length. Honour the propto bit:
  // bernoulli has no constant to drop, so the two forms agree here, but
  // hardcoding one is what made poisson_log_glm's lp land sum(log(y!))
  // away from CmdStan's.
  const auto call = [&](const auto& alpha) {
    if (ctx.variant & 0x80u)
      return stan::math::bernoulli_logit_glm_lpmf<true>(y, X, alpha,
                                                        as_rvar(ctx.in[2]));
    return stan::math::bernoulli_logit_glm_lpmf<false>(y, X, alpha,
                                                       as_rvar(ctx.in[2]));
  };
  record_probability_call([&] {
    if (ctx.in[1].len == 1) return call(rvar(ctx.in[1].data[0]));
    return call(as_rvar(ctx.in[1]));
  });
  ctx.out.data[0] = s.value;
}
// Edge order (x, alpha, beta): X is data, and edge 0 is skipped by its null
// adjoint.
void bernoulli_logit_glm_bwd(KernelCtx& ctx) { density_bwd<3>(ctx); }

// The other GLMs brms and rstanarm emit directly. A model that writes one
// of these does not merely run slower without it, it does not run.
void poisson_log_glm_fwd(KernelCtx& ctx) {
  const int64_t rows = ctx.idata[ctx.n_idata - 2];
  const int64_t cols = ctx.idata[ctx.n_idata - 1];
  Eigen::Map<const Eigen::VectorXi> y(ctx.idata, rows);
  Eigen::Map<const Eigen::MatrixXd> X(ctx.in[0].data, rows, cols);
  sink s = sink_for_args(ctx, 3);
  sink_scope active(s);
  // propto drops -lgamma(y+1), which is constant in the parameters but is
  // 10.45 on a six-observation test -- a constant offset, not noise.
  const auto call = [&](const auto& alpha) {
    if (ctx.variant & 0x80u)
      return stan::math::poisson_log_glm_lpmf<true>(y, X, alpha,
                                                    as_rvar(ctx.in[2]));
    return stan::math::poisson_log_glm_lpmf<false>(y, X, alpha,
                                                   as_rvar(ctx.in[2]));
  };
  record_probability_call([&] {
    if (ctx.in[1].len == 1) return call(rvar(ctx.in[1].data[0]));
    return call(as_rvar(ctx.in[1]));
  });
  ctx.out.data[0] = s.value;
}

void poisson_log_glm_bwd(KernelCtx& ctx) { density_bwd<3>(ctx); }

// Same, with the overdispersion argument on the end.
void neg_binomial_2_log_glm_fwd(KernelCtx& ctx) {
  const int64_t rows = ctx.idata[ctx.n_idata - 2];
  const int64_t cols = ctx.idata[ctx.n_idata - 1];
  Eigen::Map<const Eigen::VectorXi> y(ctx.idata, rows);
  Eigen::Map<const Eigen::MatrixXd> X(ctx.in[0].data, rows, cols);
  sink s = sink_for_args(ctx, 4);
  sink_scope active(s);
  const bool propto = (ctx.variant & 0x80u) != 0;
  const auto call = [&](const auto& alpha, const auto& phi) {
    if (propto)
      return stan::math::neg_binomial_2_log_glm_lpmf<true>(
          y, X, alpha, as_rvar(ctx.in[2]), phi);
    return stan::math::neg_binomial_2_log_glm_lpmf<false>(
        y, X, alpha, as_rvar(ctx.in[2]), phi);
  };
  const auto with_alpha = [&](const auto& alpha) {
    if (ctx.in[3].len == 1) return call(alpha, rvar(ctx.in[3].data[0]));
    return call(alpha, as_rvar(ctx.in[3]));
  };
  record_probability_call([&] {
    if (ctx.in[1].len == 1) return with_alpha(rvar(ctx.in[1].data[0]));
    return with_alpha(as_rvar(ctx.in[1]));
  });
  ctx.out.data[0] = s.value;
}

void neg_binomial_2_log_glm_bwd(KernelCtx& ctx) { density_bwd<4>(ctx); }

}  // namespace dens
}  // namespace stanli
