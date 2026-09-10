// docs/superpowers/plans/2026-09-10-prep-time-tuning.md
#ifndef STANLI_TUNE_HPP
#define STANLI_TUNE_HPP

#include <stanli/graph.hpp>

#include <functional>
#include <string>

namespace stanli {

struct CompiledModel;

// `g` arrives as a copy of the current graph; `alternative` rebuilds it in
// the other form and reports whether it could. `won` runs at most once, only
// if the alternative replaces the current graph.
struct TuningChoice {
  std::string what;
  std::function<bool(Graph&)> alternative;
  std::function<void()> won;
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
  double seconds = 0;
};

// Times cm.choices against cm.graph in registration order, replacing
// cm.graph with whichever alternative wins each round, then clears
// cm.choices. budget_seconds and `measurer` share the same clock.
TuneStats tune(CompiledModel& cm, double budget_seconds,
               Measurer* measurer = nullptr);

}  // namespace stanli

#endif
