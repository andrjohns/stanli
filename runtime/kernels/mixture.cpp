// log_sum_exp / log_mix.
//
// These were legacy ops whose backward replayed on `Matrix<var>`: N varis
// reached through N pointers, built and torn down per op per gradient.
// That is the AoS var tape stanli exists to avoid, sitting in the hot loop
// of every mixture and HMM model (40,636 ops across 9 corpus models).
//
// stan-math still computes every derivative here -- nothing is
// hand-differentiated. Two things changed:
//
//   * the operand is `var_value<VectorXd>` (varmat, SoA) rather than
//     `Matrix<var>`, which is what `stanc --O1` reaches for and roughly
//     twice as fast: 3.39 vs 6.68 ns/element (tools/bench_varmat.cpp), and
//   * the replay runs in the FORWARD sweep and stashes the partials, so
//     the backward is a scale of what stan-math already returned.
//
// The second point is not just bookkeeping: a backward that reads only
// scratch cannot be broken by a destructive write to its input, which is
// what puts these ops on the whitelist in inplace.cpp and lets the HMM
// forward algorithm -- fill `acc` element by element, read it whole --
// take the in-place path.
#include <stanli/graph.hpp>
#include <stanli/legacy.hpp>
#include <stanli/optable.hpp>

#include <stan/math.hpp>

namespace stanli {
namespace {

// ---- log_sum_exp over a vector --------------------------------------------
int64_t lse_scratch(const Op& op, const Slot* slots) {
  return slots[op.in[0]].len;
}

void lse_fwd(KernelCtx& ctx) {
  ctx.out.data[0] = legacy_fwd_partials_vec(
      ctx, [](const auto& x) { return stan::math::log_sum_exp(x); });
}

void lse_bwd(KernelCtx& ctx) {
  if (!ctx.in_adj[0].data) return;
  const int64_t n = ctx.in[0].len;
  for (int64_t i = 0; i < n; ++i)
    ctx.in_adj[0].data[i] += ctx.out_adj * ctx.scratch[i];
}

// ---- log_sum_exp(a, b) ----------------------------------------------------
int64_t lse2_scratch(const Op&, const Slot*) { return 2; }

void lse2_fwd(KernelCtx& ctx) {
  ctx.out.data[0] = legacy_fwd_partials_scalars<2>(ctx, [](const auto& a) {
    return stan::math::log_sum_exp(a[0], a[1]);
  });
}

void lse2_bwd(KernelCtx& ctx) {
  for (int k = 0; k < 2; ++k)
    if (ctx.in_adj[k].data)
      ctx.in_adj[k].data[0] += ctx.out_adj * ctx.scratch[k];
}

// ---- log_mix(theta, a, b) -------------------------------------------------
int64_t log_mix_scratch(const Op&, const Slot*) { return 3; }

void log_mix_fwd(KernelCtx& ctx) {
  ctx.out.data[0] = legacy_fwd_partials_scalars<3>(ctx, [](const auto& a) {
    return stan::math::log_mix(a[0], a[1], a[2]);
  });
}

void log_mix_bwd(KernelCtx& ctx) {
  for (int k = 0; k < 3; ++k)
    if (ctx.in_adj[k].data)
      ctx.in_adj[k].data[0] += ctx.out_adj * ctx.scratch[k];
}

}  // namespace

void register_mixture_kernels() {
  register_kernel(OP_LOG_SUM_EXP, Kernel{lse_fwd, lse_bwd, lse_scratch});
  register_kernel(OP_LSE2, Kernel{lse2_fwd, lse2_bwd, lse2_scratch});
  register_kernel(OP_LOG_MIX,
                  Kernel{log_mix_fwd, log_mix_bwd, log_mix_scratch});
}

}  // namespace stanli
