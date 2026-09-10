// docs/superpowers/plans/2026-09-10-prep-time-tuning.md
#ifndef STANLI_TUNE_HPP
#define STANLI_TUNE_HPP

#include <stanli/graph.hpp>

#include <functional>
#include <string>

namespace stanli {

struct CompiledModel;

// The margin inside which the cost estimate is not trusted to rank two
// forms. A property of the estimate, not of any model or machine.
inline constexpr double kTuneTrustRadius = 0.25;

// `g` arrives empty; `alternative` builds the other form into it and
// reports whether it could. `won` runs at most once, only if the
// alternative replaces the current graph. `closeness` is set at
// registration from the estimate's own costs for the decision.
struct TuningChoice {
  std::string what;
  std::function<bool(Graph&)> alternative;
  std::function<void()> won;
  double closeness = 0.0;
};

struct Measurer {
  virtual ~Measurer() = default;
  virtual double now_seconds() = 0;
};

struct TuneStats {
  int choices = 0;
  int tried = 0;
  int flipped = 0;
  int skipped_disagree = 0;
  int skipped_no_point = 0;
  int skipped_budget = 0;
  int skipped_far = 0;
  double seconds = 0;
};

// Times cm.choices against cm.graph in registration order, replacing
// cm.graph with whichever alternative wins each round, then clears
// cm.choices. The budget is one thousand times the duration of the first
// evaluation of cm.graph.
TuneStats tune(CompiledModel& cm, Measurer* measurer = nullptr);

}  // namespace stanli

#endif
