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
};

// Adaptive diagonal-metric NUTS (stan::mcmc::adapt_diag_e_nuts) over the
// executor's log_prob_grad. Returns one unconstrained parameter vector per
// post-warmup draw.
std::vector<std::vector<double>> run_nuts(Executor& ex, const NutsConfig& cfg);

}  // namespace stanli

#endif
