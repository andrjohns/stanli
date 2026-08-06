#ifndef STANLI_NUTS_HPP
#define STANLI_NUTS_HPP

#include <stanli/graph.hpp>

#include <cstdint>
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

// Adaptive diagonal-metric NUTS (stan::mcmc::adapt_diag_e_nuts) over the
// executor's log_prob_grad. Returns one unconstrained parameter vector per
// post-warmup draw.
std::vector<std::vector<double>> run_nuts(Executor& ex, const NutsConfig& cfg);

}  // namespace stanli

#endif
