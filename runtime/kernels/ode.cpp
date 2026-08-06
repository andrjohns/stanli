// ODE integrator ops. stan-math does the solving and the sensitivities; the
// right-hand side is the model's own user-defined function, evaluated at
// runtime by the MIR interpreter (see mir_eval.hpp) because the integrator
// picks the times, so the body cannot be inlined at compile time.
//
// Forward solves on doubles. Backward re-solves on a nested var tape with
// the initial state and parameters promoted, then seeds the flattened
// solution's adjoints, which is exactly what CmdStan's generated code makes
// stan-math do.
#include <stanrt/graph.hpp>
#include <stanrt/mir_eval.hpp>
#include <stanrt/ode.hpp>
#include <stanrt/optable.hpp>

#include <stan/math.hpp>

#include <vector>

namespace stanrt {
namespace {

using stan::math::var;

// Adapter presented to stan-math: evaluates the MIR body for whatever
// scalar type the integrator instantiates.
struct MirRhs {
  const OdeSpec* spec;

  template <typename T_y, typename T_param>
  std::vector<stan::return_type_t<T_y, T_param>> operator()(
      const double& t, const std::vector<T_y>& y,
      const std::vector<T_param>& theta, const std::vector<double>& x_r,
      const std::vector<int>& x_i, std::ostream* = nullptr) const {
    using T = stan::return_type_t<T_y, T_param>;
    std::vector<T> tv{T(t)}, yv(y.begin(), y.end()),
        thv(theta.begin(), theta.end()), xrv(x_r.begin(), x_r.end());
    MirEval<T> ev(*spec->funs);
    return ev.call(*spec->rhs, {tv, yv, thv, xrv}, {x_i});
  }
};

// in = {z_init, theta}; data ts / x_r / x_i and tolerances live in the spec.
// out = N_ts * S, array-major (time outer, state inner), matching Stan's
// array[N, S] layout.
template <typename T>
std::vector<std::vector<T>> solve(const OdeSpec& s, const std::vector<T>& z0,
                                  const std::vector<T>& theta) {
  MirRhs f{&s};
  if (s.stiff)
    return stan::math::integrate_ode_bdf(f, z0, s.t0, s.ts, theta, s.x_r,
                                         s.x_i, nullptr, s.rtol, s.atol,
                                         s.max_steps);
  return stan::math::integrate_ode_rk45(f, z0, s.t0, s.ts, theta, s.x_r,
                                        s.x_i, nullptr, s.rtol, s.atol,
                                        s.max_steps);
}

void ode_fwd(KernelCtx& ctx) {
  const OdeSpec& s = *static_cast<const OdeSpec*>(ctx.udata);
  std::vector<double> z0(ctx.in[0].data, ctx.in[0].data + ctx.in[0].len);
  std::vector<double> th(ctx.in[1].data, ctx.in[1].data + ctx.in[1].len);
  auto sol = solve(s, z0, th);
  const int64_t S = ctx.in[0].len;
  for (size_t n = 0; n < sol.size(); ++n)
    for (int64_t k = 0; k < S; ++k) ctx.out.data[n * S + k] = sol[n][k];
}

void ode_bwd(KernelCtx& ctx) {
  if (ctx.in_adj[0].data == nullptr && ctx.in_adj[1].data == nullptr) return;
  const OdeSpec& s = *static_cast<const OdeSpec*>(ctx.udata);
  stan::math::nested_rev_autodiff nested;
  std::vector<var> z0(ctx.in[0].data, ctx.in[0].data + ctx.in[0].len);
  std::vector<var> th(ctx.in[1].data, ctx.in[1].data + ctx.in[1].len);
  auto sol = solve(s, z0, th);
  const int64_t S = ctx.in[0].len;
  var j = 0;
  for (size_t n = 0; n < sol.size(); ++n)
    for (int64_t k = 0; k < S; ++k)
      j += ctx.out_adj_vec.data[n * S + k] * sol[n][k];
  stan::math::grad(j.vi_);
  if (ctx.in_adj[0].data)
    for (size_t i = 0; i < z0.size(); ++i)
      ctx.in_adj[0].data[i] += z0[i].adj();
  if (ctx.in_adj[1].data)
    for (size_t i = 0; i < th.size(); ++i)
      ctx.in_adj[1].data[i] += th[i].adj();
}

int64_t ode_scratch(const Op&, const Slot*) { return 0; }

}  // namespace

void register_ode_kernels() {
  register_kernel(OP_ODE, Kernel{ode_fwd, ode_bwd, ode_scratch});
}

}  // namespace stanrt
