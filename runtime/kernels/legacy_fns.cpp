// Proof-of-mechanism legacy ops. The M3 generator emits kernels of exactly
// this shape for every signature without a native port.
#include <stanrt/graph.hpp>
#include <stanrt/legacy.hpp>
#include <stanrt/optable.hpp>

namespace stanrt {
namespace {

void lse_fwd(KernelCtx& ctx) {
  Eigen::Map<const Eigen::VectorXd> x(ctx.in[0].data, ctx.in[0].len);
  ctx.out.data[0] = stan::math::log_sum_exp(x);
}
void lse_bwd(KernelCtx& ctx) {
  legacy_bwd_vec_in(ctx, [](const auto& x) {
    return stan::math::log_sum_exp(x);
  });
}

void softmax_fwd(KernelCtx& ctx) {
  Eigen::Map<const Eigen::VectorXd> x(ctx.in[0].data, ctx.in[0].len);
  Eigen::Map<Eigen::VectorXd> out(ctx.out.data, ctx.out.len);
  out = stan::math::softmax(x);
}
void softmax_bwd(KernelCtx& ctx) {
  legacy_bwd_vec_in(ctx, [](const auto& x) {
    return stan::math::softmax(x);
  });
}

// Multivariate density via nested replay: dirichlet_lpdf(theta | alpha).
// The recorder's vector edges do not model partials_vec_ (sequence-of-vector
// partials), so this is a legacy op by design.
void dirichlet_fwd(KernelCtx& ctx) {
  Eigen::Map<const Eigen::VectorXd> theta(ctx.in[0].data, ctx.in[0].len);
  Eigen::Map<const Eigen::VectorXd> alpha(ctx.in[1].data, ctx.in[1].len);
  ctx.out.data[0] = stan::math::dirichlet_lpdf<false>(theta, alpha);
}
void dirichlet_bwd(KernelCtx& ctx) {
  stan::math::nested_rev_autodiff nested;
  using stan::math::var;
  Eigen::Matrix<var, -1, 1> theta(ctx.in[0].len), alpha(ctx.in[1].len);
  for (int64_t i = 0; i < ctx.in[0].len; ++i) theta(i) = ctx.in[0].data[i];
  for (int64_t i = 0; i < ctx.in[1].len; ++i) alpha(i) = ctx.in[1].data[i];
  var lp = stan::math::dirichlet_lpdf<false>(theta, alpha);
  var j = lp * ctx.out_adj;
  stan::math::grad(j.vi_);
  if (ctx.in_adj[0].data)
    for (int64_t i = 0; i < ctx.in[0].len; ++i)
      ctx.in_adj[0].data[i] += theta(i).adj();
  if (ctx.in_adj[1].data)
    for (int64_t i = 0; i < ctx.in[1].len; ++i)
      ctx.in_adj[1].data[i] += alpha(i).adj();
}

// Binary scalar log_sum_exp via nested replay.
void lse2_fwd(KernelCtx& ctx) {
  ctx.out.data[0] =
      stan::math::log_sum_exp(ctx.in[0].data[0], ctx.in[1].data[0]);
}
void lse2_bwd(KernelCtx& ctx) {
  stan::math::nested_rev_autodiff nested;
  using stan::math::var;
  var a = ctx.in[0].data[0], b = ctx.in[1].data[0];
  var j = stan::math::log_sum_exp(a, b) * ctx.out_adj;
  stan::math::grad(j.vi_);
  if (ctx.in_adj[0].data) ctx.in_adj[0].data[0] += a.adj();
  if (ctx.in_adj[1].data) ctx.in_adj[1].data[0] += b.adj();
}

// log_mix(theta, a, b), all scalars, via nested replay.
void log_mix_fwd(KernelCtx& ctx) {
  ctx.out.data[0] = stan::math::log_mix(
      ctx.in[0].data[0], ctx.in[1].data[0], ctx.in[2].data[0]);
}
void log_mix_bwd(KernelCtx& ctx) {
  stan::math::nested_rev_autodiff nested;
  using stan::math::var;
  var t = ctx.in[0].data[0], a = ctx.in[1].data[0], b = ctx.in[2].data[0];
  var j = stan::math::log_mix(t, a, b) * ctx.out_adj;
  stan::math::grad(j.vi_);
  if (ctx.in_adj[0].data) ctx.in_adj[0].data[0] += t.adj();
  if (ctx.in_adj[1].data) ctx.in_adj[1].data[0] += a.adj();
  if (ctx.in_adj[2].data) ctx.in_adj[2].data[0] += b.adj();
}

}  // namespace

void register_legacy_kernels() {
  register_kernel(OP_LOG_SUM_EXP, Kernel{lse_fwd, lse_bwd, nullptr});
  register_kernel(OP_SOFTMAX, Kernel{softmax_fwd, softmax_bwd, nullptr});
  register_kernel(OP_DIRICHLET_LPDF,
                  Kernel{dirichlet_fwd, dirichlet_bwd, nullptr});
  register_kernel(OP_LSE2, Kernel{lse2_fwd, lse2_bwd, nullptr});
  register_kernel(OP_LOG_MIX, Kernel{log_mix_fwd, log_mix_bwd, nullptr});
}

}  // namespace stanrt
