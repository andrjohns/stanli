// The tuner's decision rule, isolated from the island carver: scripted
// batch times drive flip/keep/skip outcomes over small hand-built graphs.
// tune.cpp documents the exact now_seconds() call sequence ScriptBuilder
// replays here. budget_seconds is 1000x the scripted first-evaluation
// duration, not a parameter, so every test scripts one.
#include "env_helpers.hpp"
#include "graph_helpers.hpp"
#include <stanli/compile.hpp>
#include <stanli/graph.hpp>
#include <stanli/message_sink.hpp>
#include <stanli/optable.hpp>
#include <stanli/tune.hpp>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

static int failures = 0;
static void expect(const char* what, bool ok) {
  if (!ok) {
    ++failures;
    std::printf("FAIL %s\n", what);
  }
}
static void expect_eq(const char* what, long long got, long long want) {
  if (got != want) {
    ++failures;
    std::printf("FAIL %s got %lld want %lld\n", what, got, want);
  }
}
static void expect_exact(const char* what, double got, double want) {
  if (got != want) {
    ++failures;
    std::printf("FAIL %s got %.17g want %.17g\n", what, got, want);
  }
}
static std::string slurp(const std::string& path) {
  std::ifstream f(path);
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

using namespace stanli;

// lp = p0*p0 + p1*p0, small enough to build and evaluate for real.
static Graph build_chain() {
  Graph g;
  const int p0 = g.add_slot(1, true);
  const int p1 = g.add_slot(1, true);
  const int sq = g.add_slot(1, false);
  g.add_op(OP_MUL, {p0, p0}, sq);
  const int cross = g.add_slot(1, false);
  g.add_op(OP_MUL, {p1, p0}, cross);
  const int lp = g.add_slot(1, false);
  g.add_op(OP_ADD, {sq, cross}, lp);
  g.result_slot = lp;
  return g;
}

static CompiledModel make_model() {
  CompiledModel cm;
  cm.graph = build_chain();
  return cm;
}

static TuningChoice identity_choice(const char* what) {
  TuningChoice c;
  c.what = what;
  c.alternative = [](Graph& g) {
    g = build_chain();
    return true;
  };
  return c;
}

// Replaces the final OP_ADD with OP_SUB, so the alternative agrees with the
// current graph at the all-zero point (both terms are zero) but disagrees
// at any point where p0 and p1 are both nonzero.
static TuningChoice disagreeing_choice(const char* what) {
  TuningChoice c;
  c.what = what;
  c.alternative = [](Graph& g) {
    g = build_chain();
    for (Op& op : g.ops)
      if (op.opcode == OP_ADD) op.opcode = OP_SUB;
    return true;
  };
  return c;
}

static TuningChoice refusing_choice(const char* what) {
  TuningChoice c;
  c.what = what;
  c.alternative = [](Graph&) { return false; };
  return c;
}

// Replays the exact now_seconds() call sequence tune() makes: 64 calibration
// reads, then per choice an elapsed read, a point-0 timing pair, and (when
// rounds run) two reads per graph per round in tune()'s alternating order.
struct ScriptBuilder {
  std::vector<double> s;
  double t = 0.0;

  void resolution(double step = 1e-6) {
    for (int i = 0; i < 64; ++i) {
      s.push_back(t);
      t += step;
    }
    t -= step;
  }
  double read() {
    s.push_back(t);
    return t;
  }
  void advance(double delta) { t += delta; }

  // The one-time pre-loop timing pair that sets budget_seconds to 1000x dt.
  void budget(double dt) {
    read();
    advance(dt);
    read();
  }
  // The elapsed check plus the point-0 timing pair that sizes the batch:
  // three reads, cur_eval_time == dt.
  void calibrate(double dt) {
    read();
    read();
    advance(dt);
    read();
  }
  // The elapsed-vs-budget recheck once the alternative graph and executor
  // are built: one read, called only when the alternative did not refuse.
  void built() { read(); }
  void round_pair(double first_dt, double second_dt) {
    read();
    advance(first_dt);
    read();
    read();
    advance(second_dt);
    read();
  }
  // cur_alt_times[i] is (current batch time, alternative batch time) for
  // round i+1; tune() alternates which graph goes first by round parity.
  void rounds(const std::vector<std::pair<double, double>>& cur_alt_times) {
    for (size_t idx = 0; idx < cur_alt_times.size(); ++idx) {
      const bool current_first = ((idx + 1) % 2) == 0;
      const double cur_dt = cur_alt_times[idx].first;
      const double alt_dt = cur_alt_times[idx].second;
      if (current_first)
        round_pair(cur_dt, alt_dt);
      else
        round_pair(alt_dt, cur_dt);
    }
  }
};

struct ScriptedMeasurer : Measurer {
  std::vector<double> script;
  size_t i = 0;
  double now_seconds() override {
    if (script.empty()) return 0.0;
    const double v = i < script.size() ? script[i] : script.back();
    if (i < script.size()) ++i;
    return v;
  }
};

static void test_flips_when_faster_every_round() {
  ScriptBuilder b;
  b.resolution();
  b.budget(0.01);
  b.calibrate(0.001);
  b.built();
  b.rounds({{0.002, 0.001}, {0.002, 0.001}, {0.002, 0.001}});
  b.read();  // stats.seconds

  ScriptedMeasurer m;
  m.script = b.s;
  CompiledModel cm = make_model();
  bool won = false;
  TuningChoice c = identity_choice("flip test");
  c.won = [&] { won = true; };
  cm.choices.push_back(std::move(c));

  const TuneStats stats = tune(cm, &m);
  expect_eq("flip: choices", stats.choices, 1);
  expect_eq("flip: tried", stats.tried, 1);
  expect_eq("flip: flipped", stats.flipped, 1);
  expect("flip: won callback fired", won);
  expect_eq(
      "flip: nothing skipped",
      stats.skipped_disagree + stats.skipped_no_point + stats.skipped_budget,
      0);
}

static void test_no_flip_on_tie() {
  ScriptBuilder b;
  b.resolution();
  b.budget(0.01);
  b.calibrate(0.001);
  b.built();
  b.rounds({{0.001, 0.001},
            {0.001, 0.001},
            {0.001, 0.001},
            {0.001, 0.001},
            {0.001, 0.001},
            {0.001, 0.001},
            {0.001, 0.001}});
  b.read();

  ScriptedMeasurer m;
  m.script = b.s;
  CompiledModel cm = make_model();
  bool won = false;
  TuningChoice c = identity_choice("tie test");
  c.won = [&] { won = true; };
  cm.choices.push_back(std::move(c));

  const TuneStats stats = tune(cm, &m);
  expect_eq("tie: tried", stats.tried, 1);
  expect_eq("tie: flipped", stats.flipped, 0);
  expect("tie: won callback did not fire", !won);
}

static void test_no_flip_on_disagreement() {
  ScriptBuilder b;
  b.resolution();
  b.budget(0.01);
  b.calibrate(0.001);
  b.built();
  // Faster in every round, but the agreement gate never lets rounds start.
  b.rounds({});
  b.read();

  ScriptedMeasurer m;
  m.script = b.s;
  CompiledModel cm = make_model();
  cm.choices.push_back(disagreeing_choice("disagree test"));

  const TuneStats stats = tune(cm, &m);
  expect_eq("disagree: tried", stats.tried, 1);
  expect_eq("disagree: flipped", stats.flipped, 0);
  expect_eq("disagree: skipped_disagree", stats.skipped_disagree, 1);
}

static void test_skipped_no_point_on_refusal() {
  ScriptBuilder b;
  b.resolution();
  b.budget(0.01);
  b.calibrate(0.001);
  b.read();

  ScriptedMeasurer m;
  m.script = b.s;
  CompiledModel cm = make_model();
  cm.choices.push_back(refusing_choice("refuse test"));

  const TuneStats stats = tune(cm, &m);
  expect_eq("refuse: tried", stats.tried, 1);
  expect_eq("refuse: flipped", stats.flipped, 0);
  expect_eq("refuse: skipped_no_point", stats.skipped_no_point, 1);
}

// A choice past kTuneTrustRadius is skipped without ever calling its
// alternative; one at or under it runs the normal script.
static void test_skipped_far() {
  ScriptBuilder b;
  b.resolution();
  b.budget(0.01);
  b.calibrate(0.001);
  b.built();
  b.rounds({{0.002, 0.001}, {0.002, 0.001}, {0.002, 0.001}});
  b.read();  // stats.seconds

  ScriptedMeasurer m;
  m.script = b.s;
  CompiledModel cm = make_model();

  bool far_built = false;
  TuningChoice far;
  far.what = "far";
  far.closeness = 0.4;
  far.alternative = [&](Graph& g) {
    far_built = true;
    g = build_chain();
    return true;
  };
  cm.choices.push_back(std::move(far));

  TuningChoice close = identity_choice("close");
  close.closeness = 0.1;
  cm.choices.push_back(std::move(close));

  const TuneStats stats = tune(cm, &m);
  expect_eq("far: choices", stats.choices, 2);
  expect_eq("far: skipped_far", stats.skipped_far, 1);
  expect("far: alternative never built", !far_built);
  expect_eq("far: tried", stats.tried, 1);
}

static void test_skipped_budget() {
  ScriptBuilder b;
  b.resolution();
  // A 10us first evaluation sets budget_seconds to 0.01; a 10ms point-0 eval
  // makes one round's estimate (2 batches) far exceed that.
  b.budget(0.00001);
  b.calibrate(0.010);
  b.read();  // stats.seconds

  ScriptedMeasurer m;
  m.script = b.s;
  CompiledModel cm = make_model();
  cm.choices.push_back(identity_choice("budget test"));

  const TuneStats stats = tune(cm, &m);
  expect_eq("budget: tried", stats.tried, 0);
  expect_eq("budget: flipped", stats.flipped, 0);
  expect_eq("budget: skipped_budget", stats.skipped_budget, 1);
}

// Two choices; the second's alternative only succeeds once the first's
// `won` callback has committed, and the second's timing runs against the
// executor tune() rebuilt from the first's winning graph.
static void test_greedy_accumulation() {
  ScriptBuilder b;
  b.resolution();
  b.budget(0.01);
  b.calibrate(0.001);
  b.built();
  b.rounds({{0.002, 0.001}, {0.002, 0.001}, {0.002, 0.001}});
  b.calibrate(0.001);
  b.built();
  b.rounds({{0.002, 0.001}, {0.002, 0.001}, {0.002, 0.001}});
  b.read();

  ScriptedMeasurer m;
  m.script = b.s;
  CompiledModel cm = make_model();

  bool first_won = false;
  int second_saw_first_won = -1;

  TuningChoice first = identity_choice("first");
  first.won = [&] { first_won = true; };
  cm.choices.push_back(std::move(first));

  TuningChoice second;
  second.what = "second";
  second.alternative = [&](Graph& g) {
    g = build_chain();
    second_saw_first_won = first_won ? 1 : 0;
    return true;
  };
  cm.choices.push_back(std::move(second));

  const TuneStats stats = tune(cm, &m);
  expect_eq("greedy: choices", stats.choices, 2);
  expect_eq("greedy: tried", stats.tried, 2);
  expect_eq("greedy: flipped", stats.flipped, 2);
  expect_eq("greedy: second saw first's win", second_saw_first_won, 1);
}

static int64_t parse_choices(const std::vector<std::string>& lines) {
  for (const std::string& line : lines) {
    if (line.find("stage=tune") == std::string::npos) continue;
    const size_t pos = line.find("choices=");
    if (pos != std::string::npos) return std::atoll(line.c_str() + pos + 8);
  }
  return -1;
}

static bool idata_pointers_valid(const stanli::Graph& g) {
  for (const stanli::Op& op : g.ops) {
    if (!op.idata) continue;
    bool found = false;
    for (const std::vector<int>& block : g.idata_pool) {
      if (block.empty()) continue;
      if (op.idata >= block.data() &&
          op.idata + op.n_idata <= block.data() + block.size()) {
        found = true;
        break;
      }
    }
    if (!found) return false;
  }
  return true;
}

static void test_end_to_end_through_compile_model() {
  using namespace stanli;
  const std::string mir = slurp("tests/tune_fixtures/tune_hmm.tmir.sexp");
  const DataMap data =
      DataMap::from_json_file("tests/tune_fixtures/tune_hmm.json");

  test_setenv("STANLI_NO_TUNE", "1", 1);
  CompiledModel baseline = compile_model(mir, data);
  Executor base_ex(std::move(baseline.graph));
  baseline.bind(base_ex);
  const std::vector<double> zeros(static_cast<size_t>(base_ex.n_params()), 0.0);
  std::copy(zeros.begin(), zeros.end(), base_ex.params_data());
  std::vector<double> base_grad(zeros.size());
  const double base_lp = base_ex.gradient(base_grad.data());

  test_unsetenv("STANLI_NO_TUNE");
  test_setenv("STANLI_PROFILE_PREP", "1", 1);
  std::vector<std::string> lines;
  set_diagnostic_sink(
      [&](const char* text, size_t len) { lines.emplace_back(text, len); });
  CompiledModel tuned = compile_model(mir, data);
  set_diagnostic_sink(nullptr);
  test_unsetenv("STANLI_PROFILE_PREP");

  expect("e2e: tune stage ran with at least one choice",
         parse_choices(lines) >= 1);
  expect("e2e: idata pointers land inside tuned.graph.idata_pool",
         idata_pointers_valid(tuned.graph));

  Executor tuned_ex(std::move(tuned.graph));
  tuned.bind(tuned_ex);
  std::copy(zeros.begin(), zeros.end(), tuned_ex.params_data());
  std::vector<double> tuned_grad(zeros.size());
  const double tuned_lp = tuned_ex.gradient(tuned_grad.data());

  expect_exact("e2e: lp bitwise equal at zeros", tuned_lp, base_lp);
  expect_eq("e2e: gradient size", (long long)tuned_grad.size(),
            (long long)base_grad.size());
  for (size_t i = 0; i < base_grad.size() && i < tuned_grad.size(); ++i)
    expect_exact(("e2e: grad[" + std::to_string(i) + "] bitwise equal").c_str(),
                 tuned_grad[i], base_grad[i]);
}

int main() {
  test_flips_when_faster_every_round();
  test_no_flip_on_tie();
  test_no_flip_on_disagreement();
  test_skipped_no_point_on_refusal();
  test_skipped_far();
  test_skipped_budget();
  test_greedy_accumulation();
  test_end_to_end_through_compile_model();
  if (failures) {
    std::printf("test_tune: %d failures\n", failures);
    return 1;
  }
  std::printf("test_tune: all passed\n");
  return 0;
}
