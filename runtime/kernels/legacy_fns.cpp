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

}  // namespace

void register_legacy_kernels() {
  register_kernel(OP_LOG_SUM_EXP, Kernel{lse_fwd, lse_bwd, nullptr});
  register_kernel(OP_SOFTMAX, Kernel{softmax_fwd, softmax_bwd, nullptr});
}

}  // namespace stanrt
