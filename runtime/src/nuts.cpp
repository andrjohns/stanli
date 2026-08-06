#include <stanli/nuts.hpp>

#include <stanli/model_adapter.hpp>

#include <stan/callbacks/logger.hpp>
#include <stan/mcmc/hmc/nuts/adapt_diag_e_nuts.hpp>

#include <boost/random/additive_combine.hpp>
#include <boost/random/uniform_real_distribution.hpp>

#include <cmath>

namespace stanli {

std::vector<std::vector<double>> run_nuts(Executor& ex,
                                          const NutsConfig& cfg) {
  using rng_t = boost::ecuyer1988;
  ExecutorModel model(ex);
  rng_t rng(cfg.seed);
  stan::mcmc::adapt_diag_e_nuts<ExecutorModel, rng_t> sampler(model, rng);
  stan::callbacks::logger logger;

  const int64_t n = ex.n_params();
  Eigen::VectorXd q(n);
  boost::random::uniform_real_distribution<double> init_dist(-2.0, 2.0);
  for (int64_t i = 0; i < n; ++i) q(i) = init_dist(rng);
  sampler.seed(q);

  sampler.init_stepsize(logger);
  sampler.set_stepsize_jitter(0.0);
  sampler.get_stepsize_adaptation().set_mu(
      std::log(10.0 * sampler.get_nominal_stepsize()));
  sampler.get_stepsize_adaptation().set_delta(cfg.delta);
  sampler.set_window_params(cfg.warmup, 75, 50, 25, logger);
  sampler.engage_adaptation();

  stan::mcmc::sample s(q, 0, 0);
  for (int i = 0; i < cfg.warmup; ++i) s = sampler.transition(s, logger);
  sampler.disengage_adaptation();

  std::vector<std::vector<double>> draws;
  draws.reserve(cfg.samples);
  Eigen::VectorXd qd(n);
  for (int i = 0; i < cfg.samples; ++i) {
    s = sampler.transition(s, logger);
    s.cont_params(qd);
    draws.emplace_back(qd.data(), qd.data() + n);
  }
  return draws;
}

}  // namespace stanli
