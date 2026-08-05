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

#include <stan/math.hpp>

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
  CMapA exp_x(ctx.scratch, ctx.in[0].len);
  CMapA dout(ctx.out_adj_vec.data, ctx.out_adj_vec.len);
  if (ctx.in_adj[0].data) {
    MapA dx(ctx.in_adj[0].data, ctx.in_adj[0].len);
    dx += dout * exp_x + ctx.out2_adj;
  }
  // Parameter-dependent bound: rev lb_constrain adds ret.adj().sum().
  if (ctx.in_adj[1].data) ctx.in_adj[1].data[0] += dout.sum();
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
  CMapA exp_x(ctx.scratch, ctx.in[0].len);
  CMapA dout(ctx.out_adj_vec.data, ctx.out_adj_vec.len);
  if (ctx.in_adj[0].data) {
    MapA dx(ctx.in_adj[0].data, ctx.in_adj[0].len);
    dx += -dout * exp_x + ctx.out2_adj;
  }
  if (ctx.in_adj[1].data) ctx.in_adj[1].data[0] += dout.sum();
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
  CMapA il(ctx.scratch, ctx.in[0].len);
  CMapA dout(ctx.out_adj_vec.data, ctx.out_adj_vec.len);
  const double lb = ctx.in[1].data[0], ub = ctx.in[2].data[0];
  const double diff = ub - lb;
  if (ctx.in_adj[0].data) {
    MapA dx(ctx.in_adj[0].data, ctx.in_adj[0].len);
    dx += dout * diff * il * (1.0 - il) + ctx.out2_adj * (1.0 - 2.0 * il);
  }
  // rev lub_constrain bound adjoints (matrix-with-lp form):
  //   lb.adj += (ret.adj*(1-il)).sum() - (1/diff)*lp.adj*N
  //   ub.adj += (ret.adj*il).sum() + (1/diff)*lp.adj*N
  const double n = static_cast<double>(ctx.in[0].len);
  const double one_over_diff = 1.0 / diff;
  if (ctx.in_adj[1].data)
    ctx.in_adj[1].data[0] += (dout * (1.0 - il)).sum() +
                             -one_over_diff * ctx.out2_adj * n;
  if (ctx.in_adj[2].data)
    ctx.in_adj[2].data[0] +=
        (dout * il).sum() + one_over_diff * ctx.out2_adj * n;
}

int64_t constrain_scratch(const Op& op, const Slot* slots) {
  return slots[op.in[0]].len;
}

// Structured transforms (simplex / ordered / positive_ordered): forward runs
// the prim double implementation; backward replays the actual REV constrain
// on a nested tape with output + jacobian adjoints seeded via the dot trick.
// Correct by construction against CmdStan's own code path.
template <typename FwdF>
void structured_fwd(KernelCtx& ctx, FwdF&& f) {
  Eigen::Map<const Eigen::VectorXd> y(ctx.in[0].data, ctx.in[0].len);
  double lp = 0.0;
  Eigen::VectorXd x = f(y, lp);
  for (int64_t i = 0; i < ctx.out.len; ++i) ctx.out.data[i] = x(i);
  ctx.out2.data[0] = lp;
}
template <typename RevF>
void structured_bwd(KernelCtx& ctx, RevF&& f) {
  if (ctx.in_adj[0].data == nullptr) return;
  stan::math::nested_rev_autodiff nested;
  using stan::math::var;
  Eigen::Matrix<var, -1, 1> y(ctx.in[0].len);
  for (int64_t i = 0; i < ctx.in[0].len; ++i) y(i) = ctx.in[0].data[i];
  var lp = 0.0;
  auto x = f(y, lp);
  Eigen::Map<const Eigen::VectorXd> seed(ctx.out_adj_vec.data,
                                         ctx.out_adj_vec.len);
  var j = stan::math::dot_product(seed, x) + ctx.out2_adj * lp;
  stan::math::grad(j.vi_);
  for (int64_t i = 0; i < ctx.in[0].len; ++i)
    ctx.in_adj[0].data[i] += y(i).adj();
}

void simplex_fwd(KernelCtx& ctx) {
  structured_fwd(ctx, [](const auto& y, double& lp) {
    return stan::math::simplex_constrain(y, lp);
  });
}
void simplex_bwd(KernelCtx& ctx) {
  structured_bwd(ctx, [](auto& y, stan::math::var& lp) {
    return stan::math::simplex_constrain(y, lp);
  });
}
void ordered_fwd(KernelCtx& ctx) {
  structured_fwd(ctx, [](const auto& y, double& lp) {
    return stan::math::ordered_constrain(y, lp);
  });
}
void ordered_bwd(KernelCtx& ctx) {
  structured_bwd(ctx, [](auto& y, stan::math::var& lp) {
    return stan::math::ordered_constrain(y, lp);
  });
}
void pos_ordered_fwd(KernelCtx& ctx) {
  structured_fwd(ctx, [](const auto& y, double& lp) {
    return stan::math::positive_ordered_constrain(y, lp);
  });
}
void pos_ordered_bwd(KernelCtx& ctx) {
  structured_bwd(ctx, [](auto& y, stan::math::var& lp) {
    return stan::math::positive_ordered_constrain(y, lp);
  });
}

}  // namespace

void register_constrain_kernels() {
  register_kernel(OP_CONSTRAIN_LOWER,
                  Kernel{clower_fwd, clower_bwd, constrain_scratch});
  register_kernel(OP_CONSTRAIN_UPPER,
                  Kernel{cupper_fwd, cupper_bwd, constrain_scratch});
  register_kernel(OP_CONSTRAIN_LU,
                  Kernel{clu_fwd, clu_bwd, constrain_scratch});
  register_kernel(OP_CONSTRAIN_SIMPLEX,
                  Kernel{simplex_fwd, simplex_bwd, nullptr});
  register_kernel(OP_CONSTRAIN_ORDERED,
                  Kernel{ordered_fwd, ordered_bwd, nullptr});
  register_kernel(OP_CONSTRAIN_POS_ORDERED,
                  Kernel{pos_ordered_fwd, pos_ordered_bwd, nullptr});
}

}  // namespace stanrt
