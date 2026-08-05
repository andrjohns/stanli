// Constraint transform kernels. These mirror the REV *_constrain overloads'
// forward expressions verbatim (Eigen array ops, packet-vectorized), because
// that is what CmdStan executes for parameters; a scalar libm loop differs
// from Eigen's packet exp/inv_logit by 1 ULP on some inputs. The stored
// intermediate (exp_x / inv_logit_x) lives in scratch and is reused by
// backward, exactly as the rev overloads reuse their arena copies.
//
// out = constrained values, out2 = summed log-jacobian term.
#include <stanrt/graph.hpp>
#include <stanrt/optable.hpp>

#include <stan/math/prim.hpp>

namespace stanrt {
namespace {

using Arr = Eigen::Array<double, -1, 1>;
using MapA = Eigen::Map<Arr>;
using CMapA = Eigen::Map<const Arr>;

// rev lb_constrain(matrix, scalar, lp):
//   exp_x = x.array().exp();  ret = exp_x + lb;  lp += x.sum();
//   bwd: x.adj += ret.adj * exp_x + lp.adj
void clower_fwd(KernelCtx& ctx) {
  CMapA x(ctx.in[0].data, ctx.in[0].len);
  MapA out(ctx.out.data, ctx.out.len);
  MapA exp_x(ctx.scratch, ctx.in[0].len);
  exp_x = x.exp();
  out = exp_x + ctx.in[1].data[0];
  ctx.out2.data[0] = x.sum();
}
void clower_bwd(KernelCtx& ctx) {
  if (!ctx.in_adj[0].data) return;
  CMapA exp_x(ctx.scratch, ctx.in[0].len);
  CMapA dout(ctx.out_adj_vec.data, ctx.out_adj_vec.len);
  MapA dx(ctx.in_adj[0].data, ctx.in_adj[0].len);
  dx += dout * exp_x + ctx.out2_adj;
}

// rev ub_constrain(matrix, scalar, lp):
//   exp_x stored; ret = ub - exp_x; lp += x.sum();
//   bwd: x.adj += -ret.adj * exp_x + lp.adj
void cupper_fwd(KernelCtx& ctx) {
  CMapA x(ctx.in[0].data, ctx.in[0].len);
  MapA out(ctx.out.data, ctx.out.len);
  MapA exp_x(ctx.scratch, ctx.in[0].len);
  exp_x = x.exp();
  out = ctx.in[1].data[0] - exp_x;
  ctx.out2.data[0] = x.sum();
}
void cupper_bwd(KernelCtx& ctx) {
  if (!ctx.in_adj[0].data) return;
  CMapA exp_x(ctx.scratch, ctx.in[0].len);
  CMapA dout(ctx.out_adj_vec.data, ctx.out_adj_vec.len);
  MapA dx(ctx.in_adj[0].data, ctx.in_adj[0].len);
  dx += -dout * exp_x + ctx.out2_adj;
}

// rev lub_constrain(matrix, scalar, scalar, lp):
//   neg_abs_x = -x.abs();
//   lp += (log(diff) + (neg_abs_x - 2*log1p_exp(neg_abs_x))).sum();
//   inv_logit_x = inv_logit(x) stored; ret = diff*inv_logit_x + lb;
//   bwd: x.adj += ret.adj*diff*il*(1-il) + lp.adj*(1-2*il)
void clu_fwd(KernelCtx& ctx) {
  CMapA x(ctx.in[0].data, ctx.in[0].len);
  MapA out(ctx.out.data, ctx.out.len);
  MapA il(ctx.scratch, ctx.in[0].len);
  const double lb = ctx.in[1].data[0], ub = ctx.in[2].data[0];
  const double diff = ub - lb;
  Arr neg_abs_x = -x.abs();
  ctx.out2.data[0] =
      (std::log(diff) + (neg_abs_x - 2.0 * stan::math::log1p_exp(neg_abs_x)))
          .sum();
  il = stan::math::inv_logit(x.matrix().eval().array());
  out = diff * il + lb;
}
void clu_bwd(KernelCtx& ctx) {
  if (!ctx.in_adj[0].data) return;
  CMapA il(ctx.scratch, ctx.in[0].len);
  CMapA dout(ctx.out_adj_vec.data, ctx.out_adj_vec.len);
  MapA dx(ctx.in_adj[0].data, ctx.in_adj[0].len);
  const double lb = ctx.in[1].data[0], ub = ctx.in[2].data[0];
  const double diff = ub - lb;
  dx += dout * diff * il * (1.0 - il) + ctx.out2_adj * (1.0 - 2.0 * il);
}

int64_t constrain_scratch(const Op& op, const Slot* slots) {
  return slots[op.in[0]].len;
}

}  // namespace

void register_constrain_kernels() {
  register_kernel(OP_CONSTRAIN_LOWER,
                  Kernel{clower_fwd, clower_bwd, constrain_scratch});
  register_kernel(OP_CONSTRAIN_UPPER,
                  Kernel{cupper_fwd, cupper_bwd, constrain_scratch});
  register_kernel(OP_CONSTRAIN_LU,
                  Kernel{clu_fwd, clu_bwd, constrain_scratch});
}

}  // namespace stanrt
