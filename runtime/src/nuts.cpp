#include <stanli/nuts.hpp>

#include <stanli/model_adapter.hpp>

#include <stan/callbacks/logger.hpp>
#include <stan/mcmc/hmc/nuts/adapt_diag_e_nuts.hpp>

#include <boost/random/additive_combine.hpp>
#include <boost/random/uniform_real_distribution.hpp>

#include <cmath>
#include <stdexcept>
#include <string>

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

  // CmdStan draws uniform(-2, 2) on the unconstrained scale and REJECTS the
  // draw unless both the log density and its whole gradient are finite,
  // retrying up to 100 times (stan::services::util::initialize). We took the
  // first draw unconditionally, so a model whose typical set only covers
  // part of that hypercube -- accel_gp's GP hyperparameters overflow exp()
  // over much of it -- failed outright on seeds whose first draw landed
  // badly, with the failure surfacing later as a stepsize-search error.
  {
    constexpr int kMaxInitAttempts = 100;
    std::vector<double> grad((size_t)n);
    bool ok = false;
    for (int attempt = 0; attempt < kMaxInitAttempts && !ok; ++attempt) {
      for (int64_t i = 0; i < n; ++i) q(i) = init_dist(rng);
      for (int64_t i = 0; i < n; ++i) ex.params_data()[i] = q(i);
      const double lp = ex.gradient(grad.data());
      ok = std::isfinite(lp);
      for (int64_t i = 0; ok && i < n; ++i) ok = std::isfinite(grad[(size_t)i]);
    }
    if (!ok)
      throw std::runtime_error(
          "initialization failed: no draw in " +
          std::to_string(kMaxInitAttempts) +
          " attempts had finite log density and gradient");
  }
  sampler.seed(q);

  sampler.init_stepsize(logger);
  sampler.set_stepsize_jitter(0.0);
  sampler.get_stepsize_adaptation().set_mu(
      std::log(10.0 * sampler.get_nominal_stepsize()));
  sampler.get_stepsize_adaptation().set_delta(cfg.delta);
  sampler.set_max_depth(cfg.max_depth);
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
