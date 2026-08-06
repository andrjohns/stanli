// Legacy op mechanism: run today's stan-math rev code, unmodified, behind
// the op interface. Forward evaluates on plain doubles. Backward opens a
// nested var tape, replays the function on promoted inputs, seeds the output
// adjoints, propagates, and copies input adjoints out.
//
// Seeding uses the dot trick: grad of dot(seed, f(x)) with respect to x is
// seed^T J_f, the vjp we need, and it only touches public stan-math API.
//
// Correct by construction (it IS the current code path), expensive per call:
// the nested tape allocates from the arena, so the native-op zero-allocation
// property does not hold for legacy ops. Each one disappears from profiles
// when its function gets a native port.
#ifndef STANLI_LEGACY_HPP
#define STANLI_LEGACY_HPP

#include <stanli/graph.hpp>

#include <stan/math.hpp>

namespace stanli {

// F: Eigen var vector -> var (scalar out) or var vector (vector out).
template <typename F>
void legacy_bwd_vec_in(KernelCtx& ctx, F&& f) {
  if (ctx.in_adj[0].data == nullptr) return;
  stan::math::nested_rev_autodiff nested;
  using stan::math::var;
  Eigen::Matrix<var, -1, 1> x(ctx.in[0].len);
  for (int64_t i = 0; i < ctx.in[0].len; ++i) x(i) = ctx.in[0].data[i];
  auto out = f(x);
  var j;
  if constexpr (std::is_same_v<std::decay_t<decltype(out)>, var>) {
    j = out * ctx.out_adj;
  } else {
    Eigen::Map<const Eigen::VectorXd> seed(ctx.out_adj_vec.data,
                                           ctx.out_adj_vec.len);
    j = stan::math::dot_product(seed, out);
  }
  stan::math::grad(j.vi_);
  for (int64_t i = 0; i < ctx.in[0].len; ++i)
    ctx.in_adj[0].data[i] += x(i).adj();
}

}  // namespace stanli

#endif
