#include <stanli/nuts.hpp>

#include <stanli/model_adapter.hpp>

#include <stan/callbacks/logger.hpp>
#include <stan/mcmc/hmc/nuts/adapt_diag_e_nuts.hpp>
#include <stan/services/util/create_rng.hpp>

#include <boost/random/uniform_real_distribution.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>

namespace stanli {

std::vector<std::vector<double>> run_nuts(Executor& ex,
                                          const NutsConfig& cfg,
                                          SamplerStats* stats,
                                          const DrawObserver& observe) {
  // CmdStan's generator, seeded CmdStan's way: same engine (mixmax in this
  // Stan version, ecuyer1988 in older ones -- create_rng is what tracks
  // that), same (0, 1, seed, chain) construction, chain 1 as the default
  // `id`. Before this the seeds named unrelated streams, so "seed 1" meant
  // a different starting point in each engine and any comparison of a
  // sampling run was comparing two different draws as much as two
  // samplers. With the stream matched and the same draw order below, the
  // initial point is the same one CmdStan starts from.
  using rng_t = stan::rng_t;
  ExecutorModel model(ex);
  rng_t rng = stan::services::util::create_rng(cfg.seed, 1);
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
      // stan::io::random_var_context draws one per unconstrained
      // parameter, in declaration order, from this distribution.
      for (int64_t i = 0; i < n; ++i) q(i) = init_dist(rng);
      for (int64_t i = 0; i < n; ++i) ex.params_data()[i] = q(i);
      // A density that rejects its argument outright (stan-math throws on a
      // NaN location, an out-of-support outcome) counts as a rejected draw,
      // exactly as CmdStan's initialize treats it -- not as a fatal error.
      // CmdStan's two-stage check, in its order: the log density on
      // doubles first (stan::services::util::initialize), then the
      // gradient. The stages can disagree on an ODE model -- the value
      // path solves the states alone and the gradient path solves the
      // coupled system -- and running only the second accepted initial
      // points CmdStan rejects, which on lotka_volterra meant starting
      // in a region warmup could not climb out of.
      try {
        const double lp = ex.forward_value_only();
        ok = std::isfinite(lp);
        if (ok) {
          const double glp = ex.gradient(grad.data());
          ok = std::isfinite(glp);
          for (int64_t i = 0; ok && i < n; ++i)
            ok = std::isfinite(grad[(size_t)i]);
        }
      } catch (const std::exception&) {
        ok = false;
      }
    }
    if (!ok)
      throw std::runtime_error(
          "initialization failed: no draw in " +
          std::to_string(kMaxInitAttempts) +
          " attempts had finite log density and gradient");
  }
  if (std::getenv("STANLI_DEBUG_INIT")) {
    std::fprintf(stderr, "init");
    for (int64_t i = 0; i < n; ++i) std::fprintf(stderr, " %.17g", q(i));
    std::fprintf(stderr, "\n");
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
  Eigen::VectorXd qw(n);
  for (int i = 0; i < cfg.warmup; ++i) {
    s = sampler.transition(s, logger);
    if (observe) {
      s.cont_params(qw);
      observe(i, true, qw.data());
    }
  }
  sampler.disengage_adaptation();

  std::vector<std::vector<double>> draws;
  draws.reserve(cfg.samples);
  if (stats) {
    stats->rows.clear();
    stats->rows.reserve((size_t)cfg.samples);
  }
  Eigen::VectorXd qd(n);
  std::vector<double> sp;
  for (int i = 0; i < cfg.samples; ++i) {
    s = sampler.transition(s, logger);
    s.cont_params(qd);
    draws.emplace_back(qd.data(), qd.data() + n);
    if (observe) observe(i, false, draws.back().data());
    if (stats) {
      // get_sampler_params yields stepsize__, treedepth__, n_leapfrog__,
      // divergent__, energy__ in that order (stan::mcmc::base_nuts).
      sp.clear();
      sampler.get_sampler_params(sp);
      stats->rows.push_back({s.log_prob(), s.accept_stat(),
                             sp.size() > 0 ? sp[0] : 0.0,
                             sp.size() > 1 ? sp[1] : 0.0,
                             sp.size() > 2 ? sp[2] : 0.0,
                             sp.size() > 3 ? sp[3] : 0.0,
                             sp.size() > 4 ? sp[4] : 0.0});
    }
  }
  return draws;
}

}  // namespace stanli
