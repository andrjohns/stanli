#include <stanli/tune.hpp>

#include <stanli/compile.hpp>
#include <stanli/message_sink.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <random>
#include <vector>

namespace stanli {
namespace {

struct SteadyMeasurer : Measurer {
  using Clock = std::chrono::steady_clock;
  const Clock::time_point epoch = Clock::now();
  double now_seconds() override {
    return std::chrono::duration<double>(Clock::now() - epoch).count();
  }
};

double clock_resolution(Measurer& m, double* start) {
  double times[64];
  for (double& t : times) t = m.now_seconds();
  double best = 0.0;
  for (int i = 1; i < 64; ++i) {
    const double delta = times[i] - times[i - 1];
    if (delta > 0.0 && (best == 0.0 || delta < best)) best = delta;
  }
  *start = times[63];
  return best > 0.0 ? best : 1e-6;
}

bool finite_result(double lp, const std::vector<double>& grad) {
  if (!std::isfinite(lp)) return false;
  for (double v : grad)
    if (!std::isfinite(v)) return false;
  return true;
}

bool safe_eval(Executor& ex, const std::vector<double>& point, double* lp,
               std::vector<double>& grad) {
  std::copy(point.begin(), point.end(), ex.params_data());
  try {
    *lp = ex.gradient(grad.data());
  } catch (...) {
    return false;
  }
  return finite_result(*lp, grad);
}

double time_batch(Measurer& m, Executor& ex, const std::vector<double>& point,
                  int batch, std::vector<double>& grad) {
  std::copy(point.begin(), point.end(), ex.params_data());
  const double t0 = m.now_seconds();
  for (int i = 0; i < batch; ++i) ex.gradient(grad.data());
  return m.now_seconds() - t0;
}

double median_of(std::vector<double> v) {
  std::sort(v.begin(), v.end());
  const size_t n = v.size();
  if (n == 0) return 0.0;
  return n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

}  // namespace

TuneStats tune(CompiledModel& cm, Measurer* measurer) {
  TuneStats stats;
  if (std::getenv("STANLI_NO_TUNE")) return stats;
  stats.choices = static_cast<int>(cm.choices.size());
  if (cm.choices.empty()) return stats;

  SteadyMeasurer default_measurer;
  Measurer& m = measurer ? *measurer : default_measurer;
  const bool debug = std::getenv("STANLI_DEBUG_TUNE") != nullptr;

  double t_start = 0.0;
  const double resolution = clock_resolution(m, &t_start);
  const double batch_target = std::max(100.0 * resolution, 20e-6);

  auto cur_ex = std::make_unique<Executor>(cm.graph);
  cm.bind(*cur_ex);
  const int64_t n = cur_ex->n_params();

  std::vector<std::vector<double>> points(
      3, std::vector<double>(static_cast<size_t>(n), 0.0));
  {
    std::mt19937_64 rng(0x5ee1u);
    std::uniform_real_distribution<double> dist(-2.0, 2.0);
    for (int p = 1; p <= 2; ++p)
      for (int64_t i = 0; i < n; ++i)
        points[static_cast<size_t>(p)][static_cast<size_t>(i)] = dist(rng);
  }

  std::vector<double> cur_grad(static_cast<size_t>(n)),
      alt_grad(static_cast<size_t>(n));

  // The budget is amortized against real use: a graph that evaluates fewer
  // than a thousand gradients over its lifetime isn't worth tuning for.
  double first_lp = 0.0;
  const double t_first = m.now_seconds();
  safe_eval(*cur_ex, points[0], &first_lp, cur_grad);
  const double first_eval_time =
      std::max(m.now_seconds() - t_first, resolution);
  const double budget_seconds = 1000.0 * first_eval_time;

  for (TuningChoice& choice : cm.choices) {
    if (choice.closeness > kTuneTrustRadius) {
      ++stats.skipped_far;
      if (debug)
        emit_diagnostic("tune: " + choice.what + " skipped_far closeness=" +
                        std::to_string(choice.closeness));
      continue;
    }

    const double elapsed = m.now_seconds() - t_start;

    double p0_lp = 0.0;
    const double t_eval0 = m.now_seconds();
    const bool p0_ok = safe_eval(*cur_ex, points[0], &p0_lp, cur_grad);
    const double cur_eval_time =
        std::max(m.now_seconds() - t_eval0, resolution);
    const double p0_cur_lp = p0_lp;
    const std::vector<double> p0_cur_grad = cur_grad;

    const int batch =
        std::max(1, static_cast<int>(std::ceil(batch_target / cur_eval_time)));
    const double round_est = 2.0 * batch * cur_eval_time;
    if (elapsed + round_est > budget_seconds) {
      ++stats.skipped_budget;
      if (debug) {
        emit_diagnostic("tune: " + choice.what +
                        " skipped_budget elapsed=" + std::to_string(elapsed) +
                        " round_est=" + std::to_string(round_est) +
                        " budget=" + std::to_string(budget_seconds));
      }
      break;
    }
    ++stats.tried;

    Graph g_alt;
    if (!choice.alternative(g_alt)) {
      ++stats.skipped_no_point;
      if (debug)
        emit_diagnostic("tune: " + choice.what + " alternative refused");
      continue;
    }
    auto alt_ex = std::make_unique<Executor>(g_alt);
    cm.bind(*alt_ex);

    if (m.now_seconds() - t_start > budget_seconds) {
      ++stats.skipped_budget;
      if (debug)
        emit_diagnostic("tune: " + choice.what +
                        " skipped_budget after building alternative");
      break;
    }

    std::vector<int> usable;
    bool disagree = false;
    int disagree_point = -1, disagree_component = -1;
    for (int k = 0; k < 3 && !disagree; ++k) {
      double cur_lp, alt_lp;
      bool cur_ok;
      if (k == 0 && p0_ok) {
        cur_lp = p0_cur_lp;
        cur_grad = p0_cur_grad;
        cur_ok = true;
      } else {
        cur_ok = safe_eval(*cur_ex, points[static_cast<size_t>(k)], &cur_lp,
                           cur_grad);
      }
      const bool alt_ok =
          safe_eval(*alt_ex, points[static_cast<size_t>(k)], &alt_lp, alt_grad);
      if (!cur_ok || !alt_ok) continue;
      if (cur_lp != alt_lp) {
        disagree = true;
        disagree_point = k;
        break;
      }
      for (int64_t j = 0; j < n; ++j) {
        if (cur_grad[static_cast<size_t>(j)] !=
            alt_grad[static_cast<size_t>(j)]) {
          disagree = true;
          disagree_point = k;
          disagree_component = static_cast<int>(j);
          break;
        }
      }
      if (disagree) break;
      usable.push_back(k);
    }

    if (disagree) {
      ++stats.skipped_disagree;
      if (debug) {
        std::string msg = "tune: " + choice.what + " disagrees at point " +
                          std::to_string(disagree_point);
        msg += disagree_component >= 0
                   ? " component " + std::to_string(disagree_component)
                   : " lp";
        emit_diagnostic(msg);
      }
      continue;
    }
    if (usable.empty()) {
      ++stats.skipped_no_point;
      if (debug) emit_diagnostic("tune: " + choice.what + " no usable point");
      continue;
    }

    std::vector<double> cur_times, alt_times;
    int cur_faster = 0, alt_faster = 0;
    bool decided = false, alt_wins = false;
    for (int round = 1; round <= 7; ++round) {
      const int point =
          usable[static_cast<size_t>((round - 1) % (int)usable.size())];
      const bool current_first = round % 2 == 0;
      double ct, at;
      if (current_first) {
        ct = time_batch(m, *cur_ex, points[static_cast<size_t>(point)], batch,
                        cur_grad);
        at = time_batch(m, *alt_ex, points[static_cast<size_t>(point)], batch,
                        alt_grad);
      } else {
        at = time_batch(m, *alt_ex, points[static_cast<size_t>(point)], batch,
                        alt_grad);
        ct = time_batch(m, *cur_ex, points[static_cast<size_t>(point)], batch,
                        cur_grad);
      }
      cur_times.push_back(ct);
      alt_times.push_back(at);
      if (at < ct)
        ++alt_faster;
      else if (ct < at)
        ++cur_faster;

      if (round == 3) {
        if (alt_faster == 3 && median_of(alt_times) < median_of(cur_times)) {
          decided = true;
          alt_wins = true;
          break;
        }
        if (cur_faster == 3) {
          decided = true;
          alt_wins = false;
          break;
        }
      }
    }
    if (!decided)
      alt_wins = alt_faster >= 5 && median_of(alt_times) < median_of(cur_times);

    if (debug) {
      emit_diagnostic("tune: " + choice.what + (alt_wins ? " flip" : " keep") +
                      " alt_faster=" + std::to_string(alt_faster) +
                      " cur_faster=" + std::to_string(cur_faster) +
                      " median_cur=" + std::to_string(median_of(cur_times)) +
                      " median_alt=" + std::to_string(median_of(alt_times)));
    }

    if (alt_wins) {
      ++stats.flipped;
      cm.graph = std::move(g_alt);
      if (choice.won) choice.won();
      cur_ex = std::move(alt_ex);
    }
  }

  stats.seconds = m.now_seconds() - t_start;
  cm.choices.clear();
  return stats;
}

}  // namespace stanli
