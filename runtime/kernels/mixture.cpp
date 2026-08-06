// log_sum_exp / log_mix, natively.
//
// These were legacy ops: their backward spun up a `nested_rev_autodiff`,
// built vars, called grad(), and tore the tape down again -- per scalar op,
// per gradient evaluation. That is the pointer-chasing var tape stanli
// exists to avoid, reintroduced in the hot loop of every mixture and HMM
// model (40,636 of these ops across 9 corpus models).
//
// The partials are elementary, so the forward stashes them in scratch the
// way the density kernels do and the backward becomes a few multiply-adds.
// That also makes the backward independent of the input VALUES, which lets
// these ops join the destructive-update whitelist in inplace.cpp -- the
// HMM forward algorithm writes `acc[j]` element by element and reads it
// whole, exactly the pattern that whitelist gates.
//
//   log_sum_exp(x)      out = m + log(sum exp(x-m)),  d/dx_i = exp(x_i-out)
//   log_sum_exp(a,b)    the same, two scalars
//   log_mix(t,a,b)      out = log(t*e^a + (1-t)*e^b)
//                       d/dt = e^(a-out) - e^(b-out)
//                       d/da = e^(log(t)+a-out),  d/db = e^(log1m(t)+b-out)
#include <stanli/graph.hpp>
#include <stanli/optable.hpp>

#include <stan/math/prim.hpp>

#include <cmath>

namespace stanli {
namespace {

// ---- log_sum_exp over a vector --------------------------------------------
int64_t lse_scratch(const Op& op, const Slot* slots) {
  return slots[op.in[0]].len;
}

void lse_fwd(KernelCtx& ctx) {
  const int64_t n = ctx.in[0].len;
  const double* x = ctx.in[0].data;
  const double out = stan::math::log_sum_exp(
      Eigen::Map<const Eigen::VectorXd>(x, n));
  ctx.out.data[0] = out;
  // exp(x_i - out) is the softmax, and the partial. Infinite inputs give
  // the same 0/1 split stan-math's rev overload produces.
  for (int64_t i = 0; i < n; ++i) ctx.scratch[i] = std::exp(x[i] - out);
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
  const double a = ctx.in[0].data[0], b = ctx.in[1].data[0];
  const double out = stan::math::log_sum_exp(a, b);
  ctx.out.data[0] = out;
  ctx.scratch[0] = std::exp(a - out);
  ctx.scratch[1] = std::exp(b - out);
}

void lse2_bwd(KernelCtx& ctx) {
  if (ctx.in_adj[0].data)
    ctx.in_adj[0].data[0] += ctx.out_adj * ctx.scratch[0];
  if (ctx.in_adj[1].data)
    ctx.in_adj[1].data[0] += ctx.out_adj * ctx.scratch[1];
}

// ---- log_mix(theta, a, b) -------------------------------------------------
int64_t log_mix_scratch(const Op&, const Slot*) { return 3; }

void log_mix_fwd(KernelCtx& ctx) {
  const double t = ctx.in[0].data[0];
  const double a = ctx.in[1].data[0], b = ctx.in[2].data[0];
  const double out = stan::math::log_mix(t, a, b);
  ctx.out.data[0] = out;
  ctx.scratch[0] = std::exp(a - out) - std::exp(b - out);       // d/dtheta
  ctx.scratch[1] = std::exp(std::log(t) + a - out);             // d/da
  ctx.scratch[2] = std::exp(stan::math::log1m(t) + b - out);    // d/db
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
