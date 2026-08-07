// ODE integrator ops. stan-math does the solving and the sensitivities; the
// right-hand side is the model's own user-defined function, evaluated at
// runtime by the MIR interpreter (see mir_interp.hpp) because the integrator
// picks the times, so the body cannot be inlined at compile time.
//
// One solve per gradient, in the forward sweep, with the solution's jacobian
// stashed for the backward one.
//
// The forward pass has to solve the coupled system (states plus
// sensitivities) rather than the plain state system: the adaptive step
// controller sees the coupled error estimate, so at the loose tolerances
// these models use a states-only solve lands on visibly different values
// (measured 3e-2 relative on lotka_volterra, whose atol is 1e-3). Since it
// pays for the sensitivities anyway, it may as well keep them -- the
// backward is then a matrix-vector product instead of a second solve.
//
// Reading them out is cheap. stan-math integrates the coupled system on
// doubles and only builds precomputed-gradient varis for the solution
// itself, so the nested tape left standing after a solve holds the inputs
// and the outputs and nothing else; one reverse sweep per output element
// walks a few dozen varis, against several hundred right-hand-side
// evaluations for a solve.
#include <stanli/graph.hpp>
#include <stanli/mir_interp.hpp>
#include <stanli/ode.hpp>
#include <stanli/optable.hpp>

#include <stan/math.hpp>

#include <vector>

namespace stanli {
namespace {

using stan::math::var;

// Adapter presented to stan-math: evaluates the right-hand side for whatever
// scalar type the integrator instantiates. The compiled program when there is
// one, the MIR interpreter when there is not.
struct MirRhs {
  const OdeSpec* spec;

  template <typename T_y, typename T_param>
  std::vector<stan::return_type_t<T_y, T_param>> operator()(
      const double& t, const std::vector<T_y>& y,
      const std::vector<T_param>& theta, const std::vector<double>& x_r,
      const std::vector<int>& x_i, std::ostream* = nullptr) const {
    using T = stan::return_type_t<T_y, T_param>;
    if (spec->prog.ok) {
      // y and theta arrive as T_y / T_param, which are T or double; the
      // register file is T, so promote through a small staging buffer only
      // when they differ.
      std::vector<T> out, ys(y.begin(), y.end()), ths(theta.begin(),
                                                      theta.end());
      run_rhs<T>(spec->prog, T(t), ys.data(), ths.data(), x_r.data(), out);
      return out;
    }
    std::vector<T> tv{T(t)}, yv(y.begin(), y.end()),
        thv(theta.begin(), theta.end()), xrv(x_r.begin(), x_r.end());
    MirInterp<T> ev(*spec->funs(), "ODE function");
    return ev.call(*spec->rhs(), {tv, yv, thv, xrv}, {x_i});
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
  const int64_t S = ctx.in[0].len, P = ctx.in[1].len, W = S + P;
  stan::math::nested_rev_autodiff nested;
  std::vector<var> z0(ctx.in[0].data, ctx.in[0].data + S);
  std::vector<var> th(ctx.in[1].data, ctx.in[1].data + P);
  auto solv = solve(s, z0, th);
  for (size_t n = 0; n < solv.size(); ++n)
    for (int64_t k = 0; k < S; ++k)
      ctx.out.data[(int64_t)n * S + k] = solv[n][k].val();

  // d(solution)/d(z_init, theta), row per flattened solution element. Swept
  // last element first so the adjoint accumulation in ode_bwd runs in the
  // same order the reverse sweep over these varis used to, which keeps the
  // gradient bit-identical to what a second solve produced.
  double* J = ctx.scratch;
  for (int64_t o = ctx.out.len; o-- > 0;) {
    stan::math::set_zero_all_adjoints_nested();
    stan::math::grad(solv[(size_t)(o / S)][(size_t)(o % S)].vi_);
    for (int64_t i = 0; i < S; ++i) J[o * W + i] = z0[(size_t)i].adj();
    for (int64_t i = 0; i < P; ++i) J[o * W + S + i] = th[(size_t)i].adj();
  }
}

void ode_bwd(KernelCtx& ctx) {
  if (ctx.in_adj[0].data == nullptr && ctx.in_adj[1].data == nullptr) return;
  const int64_t S = ctx.in[0].len, P = ctx.in[1].len, W = S + P;
  const double* J = ctx.scratch;
  for (int64_t o = ctx.out.len; o-- > 0;) {
    const double a = ctx.out_adj_vec.data[o];
    if (ctx.in_adj[0].data)
      for (int64_t i = 0; i < S; ++i) ctx.in_adj[0].data[i] += a * J[o * W + i];
    if (ctx.in_adj[1].data)
      for (int64_t i = 0; i < P; ++i)
        ctx.in_adj[1].data[i] += a * J[o * W + S + i];
  }
}

int64_t ode_scratch(const Op& op, const Slot* slots) {
  return slots[op.out].len * (slots[op.in[0]].len + slots[op.in[1]].len);
}

}  // namespace

void register_ode_kernels() {
  register_kernel(OP_ODE, Kernel{ode_fwd, ode_bwd, ode_scratch});
}

}  // namespace stanli
