#ifndef STANLI_NUTS_HPP
#define STANLI_NUTS_HPP

#include <stanli/graph.hpp>

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

namespace stanli {

struct NutsConfig {
  uint32_t seed = 0;
  int warmup = 1000;
  int samples = 1000;
  double delta = 0.8;  // target acceptance statistic
  // stan::mcmc::base_nuts defaults this to 5; CmdStan sets 10 and so must
  // we, or trajectories cap at 31 leapfrogs instead of 1023 and any model
  // needing deep trees is silently under-explored.
  int max_depth = 10;
};

// One row per post-warmup draw, in CmdStan's column order:
//   lp__, accept_stat__, stepsize__, treedepth__, n_leapfrog__,
//   divergent__, energy__
// This is the sampler-level oracle the gradient rig cannot be: comparing
// these against a CmdStan run catches configuration divergence (a wrong
// max tree depth, a wrong adaptation target) that pointwise gradient
// verification is structurally blind to.
struct SamplerStats {
  std::vector<std::array<double, 7>> rows;
};

// Optional per-transition observer for streaming consumers (the browser
// worker's live plots): called after every transition with the phase and
// the current unconstrained point.
using DrawObserver =
    std::function<void(int64_t i, bool warmup, const double* q)>;

// Adaptive diagonal-metric NUTS (stan::mcmc::adapt_diag_e_nuts) over the
// executor's log_prob_grad. Returns one unconstrained parameter vector per
// post-warmup draw. `stats`, when non-null, receives one row per draw.
std::vector<std::vector<double>> run_nuts(Executor& ex, const NutsConfig& cfg,
                                          SamplerStats* stats = nullptr,
                                          const DrawObserver& observe = {});

}  // namespace stanli

#endif
