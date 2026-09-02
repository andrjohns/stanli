#include "env_helpers.hpp"

#include <stanli/compile.hpp>
#include <stanli/graph.hpp>
#include <stanli/optable.hpp>
#include <stanli/structured_loop.hpp>
#include <stan/math.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace stanli;
using Node = StructuredLoop::Node;

static int failures = 0;

static bool near(double actual, double expected) {
  const double tolerance =
      1e-12 * std::max({1.0, std::fabs(actual), std::fabs(expected)});
  return actual == expected ||
         (std::isfinite(actual) && std::isfinite(expected) &&
          std::fabs(actual - expected) <= tolerance);
}

static void check(bool ok, const char* name) {
  if (!ok) {
    ++failures;
    std::printf("FAIL %s\n", name);
  }
}

static void close(double actual, double expected, const char* name) {
  if (!near(actual, expected)) {
    std::printf("  %.17g != %.17g\n", actual, expected);
    check(false, name);
  }
}

static Node sequence(std::vector<Node> children) {
  Node node;
  node.children = std::move(children);
  return node;
}

static Node call(StructuredLoop& plan, uint16_t opcode,
                 std::initializer_list<int> inputs, int output) {
  Node node;
  node.kind = Node::KernelCall;
  node.op = plan.body.add_op(opcode, inputs, output);
  return node;
}

static Node alias(int destination, int source) {
  Node node;
  node.kind = Node::Alias;
  node.dst = destination;
  node.src = source;
  return node;
}

static int scalar(StructuredLoop& plan, double value) {
  const int slot = plan.body.add_slot(1, false);
  plan.fills.push_back({slot, {value}});
  return slot;
}

static Node counted(int lower, int upper, int iterator, int64_t capacity,
                    Node body) {
  Node node;
  node.kind = Node::For;
  node.lower = lower;
  node.upper = upper;
  node.iterator = iterator;
  node.capacity = capacity;
  node.children.push_back(std::move(body));
  return node;
}

static Node branch(int condition, Node yes, Node no) {
  Node node;
  node.kind = Node::If;
  node.condition = condition;
  node.children = {std::move(yes), std::move(no)};
  return node;
}

static Node while_loop(int condition, int64_t capacity, Node condition_body,
                       Node body) {
  Node node;
  node.kind = Node::While;
  node.condition = condition;
  node.capacity = capacity;
  node.children = {std::move(condition_body), std::move(body)};
  return node;
}

static bool set_forward(Node& node, int op, void (*forward)(KernelCtx&)) {
  if (node.kind == Node::KernelCall && node.op == op) {
    node.forward = forward;
    return true;
  }
  for (auto& child : node.children)
    if (set_forward(child, op, forward)) return true;
  return false;
}

static std::shared_ptr<StructuredLoop> recurrence(int trips) {
  auto plan = std::make_shared<StructuredLoop>();
  for (int i = 0; i < 7; ++i) plan->body.add_slot(1, false);
  plan->imports = {{0, 0, 0}, {1, 1, 0}};
  const int lower = scalar(*plan, 1);
  const int upper = scalar(*plan, trips);
  plan->root = sequence(
      {alias(2, 0),
       counted(lower, upper, 3, trips,
               sequence({call(*plan, OP_MUL, {2, 1}, 4),
                         call(*plan, OP_ADD, {4, 0}, 5),
                         call(*plan, OP_TANHV, {5}, 6), alias(2, 6)}))});
  plan->outputs = {2};
  plan->prepare(256LL << 20);
  return plan;
}

static Graph outer(std::shared_ptr<StructuredLoop> plan) {
  Graph graph;
  graph.add_slot(1, true);
  graph.add_slot(1, true);
  const int output = graph.add_slot(1, false);
  const int op = graph.add_op(OP_LOOP, {0, 1}, output);
  graph.ops[op].udata = plan.get();
  graph.udata_pool.push_back(std::move(plan));
  graph.result_slot = output;
  return graph;
}

static void runtime_trip_tests() {
  for (int trips : {0, 1, 8}) {
    auto plan = recurrence(trips);
    check(plan->node_count < 16, "loop code size is independent of trips");
    Executor executor(outer(plan));
    for (const auto& point :
         std::vector<std::pair<double, double>>{{.1, .7}, {-.2, .3}}) {
      executor.params_data()[0] = point.first;
      executor.params_data()[1] = point.second;
      double gradient[2];
      const double value = executor.gradient(gradient);

      stan::math::nested_rev_autodiff nested;
      stan::math::var theta = point.first;
      stan::math::var beta = point.second;
      stan::math::var state = theta;
      for (int i = 0; i < trips; ++i)
        state = stan::math::tanh(state * beta + theta);
      state.grad();
      close(value, state.val(), "zero/one/many trip value");
      close(gradient[0], theta.adj(), "zero/one/many theta gradient");
      close(gradient[1], beta.adj(), "zero/one/many beta gradient");
    }
  }
}

static std::string fixture_mir(const std::string& name) {
  std::ifstream input("tests/fixtures/" + name + ".tmir.sexp");
  check(bool(input), "generated MIR fixture is readable");
  std::ostringstream text;
  text << input.rdbuf();
  return text.str();
}

enum class Mode { Auto, Off, Force };

static CompiledModel compile_fixture(const std::string& name, int trips,
                                     Mode mode) {
  if (mode == Mode::Auto)
    test_unsetenv("STANLI_STRUCTURED_LOOPS");
  else
    test_setenv("STANLI_STRUCTURED_LOOPS", mode == Mode::Off ? "0" : "force");
  DataMap data;
  data.set_int("N", trips);
  return compile_model(fixture_mir(name), data);
}

static const StructuredLoop* retained(const CompiledModel& model) {
  for (const auto& op : model.graph.ops)
    if (op.opcode == OP_LOOP)
      return static_cast<const StructuredLoop*>(op.udata);
  return nullptr;
}

static bool has_kind(const Node& node, Node::Kind kind) {
  if (node.kind == kind) return true;
  for (const auto& child : node.children)
    if (has_kind(child, kind)) return true;
  return false;
}

static void check_native_only(const StructuredLoop& plan, const char* name) {
  for (const auto& op : plan.body.ops) check(op.opcode != OP_ISLAND, name);
}

static void compare_gradients(const CompiledModel& native,
                              const CompiledModel& legacy,
                              const std::vector<std::vector<double>>& points,
                              const char* value_name,
                              const char* gradient_name) {
  check(native.n_unconstrained == legacy.n_unconstrained,
        "differential parameter count");
  Executor a(native.graph), b(legacy.graph);
  native.bind(a);
  legacy.bind(b);
  std::vector<double> ga(static_cast<size_t>(native.n_unconstrained));
  std::vector<double> gb(static_cast<size_t>(legacy.n_unconstrained));
  for (const auto& point : points) {
    check(point.size() == ga.size(), "differential point width");
    std::copy(point.begin(), point.end(), a.params_data());
    std::copy(point.begin(), point.end(), b.params_data());
    close(a.gradient(ga.data()), b.gradient(gb.data()), value_name);
    for (size_t i = 0; i < ga.size(); ++i) close(ga[i], gb[i], gradient_name);
  }
}

static bool same_graph_shape(const Graph& a, const Graph& b) {
  if (a.slots.size() != b.slots.size() || a.ops.size() != b.ops.size() ||
      a.result_slot != b.result_slot ||
      a.integer_storage_size() != b.integer_storage_size())
    return false;
  for (size_t i = 0; i < a.slots.size(); ++i)
    if (a.slots[i].offset != b.slots[i].offset ||
        a.slots[i].len != b.slots[i].len ||
        a.slots[i].is_param != b.slots[i].is_param)
      return false;
  for (size_t i = 0; i < a.ops.size(); ++i) {
    const auto& x = a.ops[i];
    const auto& y = b.ops[i];
    if (x.opcode != y.opcode || x.variant != y.variant || x.out != y.out ||
        x.out2 != y.out2 || x.n_in != y.n_in || x.n_idata != y.n_idata ||
        bool(x.udata) != bool(y.udata))
      return false;
    for (int input = 0; input < x.n_in; ++input)
      if (x.in[input] != y.in[input]) return false;
    for (int64_t data = 0; data < x.n_idata; ++data)
      if (x.idata[data] != y.idata[data]) return false;
  }
  return true;
}

static void forced_control_tests() {
  const auto nested = compile_fixture("structured_nested", 5, Mode::Force);
  const auto nested_legacy = compile_fixture("structured_nested", 5, Mode::Off);
  const StructuredLoop* nested_plan = retained(nested);
  check(nested_plan != nullptr, "forced nested control retains OP_LOOP");
  if (nested_plan) {
    check(has_kind(nested_plan->root, Node::For), "native nested for");
    check(has_kind(nested_plan->root, Node::While), "native nested while");
    check(has_kind(nested_plan->root, Node::If), "native nested if");
    check_native_only(*nested_plan, "nested body has no replay op");
  }
  compare_gradients(nested, nested_legacy, {{.1, .7}, {-.2, .3}, {0, .5}},
                    "nested target parity", "nested gradient parity");

  const auto exits = compile_fixture("structured_exits", 6, Mode::Force);
  const auto exits_legacy = compile_fixture("structured_exits", 6, Mode::Off);
  const StructuredLoop* exits_plan = retained(exits);
  check(exits_plan != nullptr, "forced exits retain OP_LOOP");
  if (exits_plan) {
    check(has_kind(exits_plan->root, Node::For), "native nested counted loop");
    check(has_kind(exits_plan->root, Node::Break), "native break");
    check(has_kind(exits_plan->root, Node::Continue), "native continue");
    check_native_only(*exits_plan, "exit body has no replay op");
  }
  compare_gradients(exits, exits_legacy, {{.1, .7}, {-.2, .3}},
                    "exit target parity", "exit gradient parity");
}

static void dynamic_history_tests() {
  // Force even this small retained body onto the reached-frame tape used by
  // large loops. Multiple points exercise both reuse after reverse and the
  // target-fragment path without requiring a multi-gigabyte test fixture.
  test_setenv("STANLI_STRUCTURED_HISTORY_BYTES", "1");
  const auto dynamic = compile_fixture("structured_nested", 5, Mode::Force);
  test_unsetenv("STANLI_STRUCTURED_HISTORY_BYTES");
  const StructuredLoop* plan = retained(dynamic);
  check(plan != nullptr, "dynamic-history fixture retains OP_LOOP");
  if (plan) {
    check(plan->dynamic_history, "tiny history budget selects reached tape");
    check_native_only(*plan, "dynamic-history body is native-only");
  }
  const auto legacy = compile_fixture("structured_nested", 5, Mode::Off);
  compare_gradients(dynamic, legacy, {{.1, .7}, {-.2, .3}, {0, .5}},
                    "dynamic-history target parity",
                    "dynamic-history gradient parity");
}

struct Evaluation {
  double value = 0;
  double gradient[2] = {0, 0};
};

static std::atomic<int> invariant_first_calls{0};
static std::atomic<int> invariant_second_calls{0};
static std::atomic<int> invariant_active_calls{0};
static std::atomic<int> invariant_variant_calls{0};

static void count_invariant_first(KernelCtx& context) {
  ++invariant_first_calls;
  find_kernel(OP_ADD)->forward(context);
}
static void count_invariant_second(KernelCtx& context) {
  ++invariant_second_calls;
  find_kernel(OP_MUL)->forward(context);
}
static void count_invariant_second_add(KernelCtx& context) {
  ++invariant_second_calls;
  find_kernel(OP_ADD)->forward(context);
}
static void count_invariant_active(KernelCtx& context) {
  ++invariant_active_calls;
  find_kernel(OP_ADD)->forward(context);
}
static void count_invariant_variant(KernelCtx& context) {
  ++invariant_variant_calls;
  find_kernel(OP_ADD)->forward(context);
}

static std::shared_ptr<StructuredLoop> invariant_plan(int trips) {
  auto plan = std::make_shared<StructuredLoop>();
  const int theta = plan->body.add_slot(1, false);
  const int lower = scalar(*plan, 1);
  const int upper = scalar(*plan, trips);
  const int iterator = plan->body.add_slot(1, false);
  const int two = scalar(*plan, 2);
  const int three = scalar(*plan, 3);
  const int invariant_first = plan->body.add_slot(1, false);
  const int invariant_second = plan->body.add_slot(1, false);
  const int active = plan->body.add_slot(1, false);
  const int variant = plan->body.add_slot(1, false);
  const int result = plan->body.add_slot(1, false);
  plan->imports = {{theta, 0, 0}};
  Node first = call(*plan, OP_ADD, {two, three}, invariant_first);
  const int first_op = first.op;
  Node second = call(*plan, OP_MUL, {invariant_first, two}, invariant_second);
  const int second_op = second.op;
  Node active_call = call(*plan, OP_ADD, {result, invariant_second}, active);
  const int active_op = active_call.op;
  Node variant_call = call(*plan, OP_ADD, {iterator, two}, variant);
  const int variant_op = variant_call.op;
  plan->root = sequence(
      {alias(result, theta),
       counted(lower, upper, iterator, std::max(0, trips),
               sequence({std::move(first), std::move(second),
                         std::move(active_call), alias(result, active),
                         std::move(variant_call)}))});
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  check(set_forward(plan->root, first_op, count_invariant_first),
        "find first invariant callback");
  check(set_forward(plan->root, second_op, count_invariant_second),
        "find transitive invariant callback");
  check(set_forward(plan->root, active_op, count_invariant_active),
        "find active callback");
  check(set_forward(plan->root, variant_op, count_invariant_variant),
        "find iterator-dependent callback");
  return plan;
}

static Evaluation evaluate_invariant(Executor& executor, double theta) {
  executor.params_data()[0] = theta;
  executor.params_data()[1] = 0;
  Evaluation result;
  result.value = executor.gradient(result.gradient);
  return result;
}

static void loop_invariant_reuse_tests() {
  test_unsetenv("STANLI_NO_STRUCTURED_INVARIANT_REUSE");
  auto plan = invariant_plan(3);
  Executor enabled(outer(plan));
  invariant_first_calls = invariant_second_calls = 0;
  invariant_active_calls = invariant_variant_calls = 0;
  const Evaluation first = evaluate_invariant(enabled, .25);
  check(invariant_first_calls == 1 && invariant_second_calls == 1,
        "inactive invariant chain executes once per loop invocation");
  check(invariant_active_calls == 3,
        "active loop work retains every reverse record");
  check(invariant_variant_calls == 3,
        "iterator-dependent work is not reused");
  const Evaluation second = evaluate_invariant(enabled, .25);
  check(invariant_first_calls == 2 && invariant_second_calls == 2,
        "invariant cache resets for a new forward evaluation");
  check(std::memcmp(&first, &second, sizeof(Evaluation)) == 0,
        "repeated invariant evaluation is bitwise stable");

  test_setenv("STANLI_NO_STRUCTURED_INVARIANT_REUSE", "1");
  Executor disabled(outer(plan));
  invariant_first_calls = invariant_second_calls = 0;
  invariant_active_calls = invariant_variant_calls = 0;
  const Evaluation baseline = evaluate_invariant(disabled, .25);
  test_unsetenv("STANLI_NO_STRUCTURED_INVARIANT_REUSE");
  check(invariant_first_calls == 3 && invariant_second_calls == 3,
        "invariant ablation executes every reached callback");
  check(std::memcmp(&first, &baseline, sizeof(Evaluation)) == 0,
        "invariant reuse has bitwise same-binary parity");

  auto zero = invariant_plan(0);
  Executor zero_executor(outer(zero));
  invariant_first_calls = invariant_second_calls = 0;
  const Evaluation zero_result = evaluate_invariant(zero_executor, .25);
  check(invariant_first_calls == 0 && invariant_second_calls == 0,
        "zero-trip loop does not pre-execute invariants");
  close(zero_result.value, .25, "zero-trip invariant value");

  // A downstream site can run before a conditional invariant is first
  // reached. Exact input-handle matching must refresh it when that definition
  // appears, then permit reuse.
  auto late = std::make_shared<StructuredLoop>();
  const int late_theta = late->body.add_slot(1, false);
  const int late_lower = scalar(*late, 1);
  const int late_upper = scalar(*late, 3);
  const int late_iterator = late->body.add_slot(1, false);
  const int late_one = scalar(*late, 1);
  const int late_condition = late->body.add_slot(1, false);
  const int late_source = late->body.add_slot(1, false);
  const int late_result = late->body.add_slot(1, false);
  late->imports = {{late_theta, 0, 0}};
  Node comparison = call(*late, OP_COMPARE, {late_iterator, late_one},
                         late_condition);
  late->body.ops[static_cast<size_t>(comparison.op)].variant = 2;
  Node late_definition = call(*late, OP_ADD, {late_one, late_one}, late_source);
  const int late_definition_op = late_definition.op;
  Node late_use = call(*late, OP_ADD, {late_source, late_one}, late_result);
  const int late_use_op = late_use.op;
  late->root = counted(
      late_lower, late_upper, late_iterator, 3,
      sequence({std::move(comparison),
                branch(late_condition, std::move(late_definition), sequence({})),
                std::move(late_use)}));
  late->outputs = {late_result};
  late->prepare(1 << 20);
  late->dynamic_history = true;
  check(set_forward(late->root, late_definition_op, count_invariant_first),
        "find late invariant definition");
  check(set_forward(late->root, late_use_op, count_invariant_second_add),
        "find late invariant consumer");
  invariant_first_calls = invariant_second_calls = 0;
  Executor late_executor(outer(late));
  const Evaluation late_result_value = evaluate_invariant(late_executor, 0);
  close(late_result_value.value, 3,
        "invariant cache observes a late conditional definition");
  check(invariant_first_calls == 1 && invariant_second_calls == 2,
        "changed input handles refresh a downstream invariant cache");

  auto while_plan = std::make_shared<StructuredLoop>();
  const int while_theta = while_plan->body.add_slot(1, false);
  const int while_counter = scalar(*while_plan, 0);
  const int while_one = scalar(*while_plan, 1);
  const int while_three = scalar(*while_plan, 3);
  const int while_condition = while_plan->body.add_slot(1, false);
  const int while_next = while_plan->body.add_slot(1, false);
  const int while_invariant = while_plan->body.add_slot(1, false);
  const int while_result = while_plan->body.add_slot(1, false);
  while_plan->imports = {{while_theta, 0, 0}};
  Node while_compare = call(*while_plan, OP_COMPARE,
                            {while_three, while_counter}, while_condition);
  while_plan->body.ops[static_cast<size_t>(while_compare.op)].variant = 2;
  Node while_constant =
      call(*while_plan, OP_ADD, {while_one, while_one}, while_invariant);
  const int while_constant_op = while_constant.op;
  while_plan->root = while_loop(
      while_condition, 3, std::move(while_compare),
      sequence({std::move(while_constant),
                call(*while_plan, OP_ADD, {while_counter, while_one}, while_next),
                alias(while_counter, while_next),
                alias(while_result, while_invariant)}));
  while_plan->outputs = {while_result};
  while_plan->prepare(1 << 20);
  while_plan->dynamic_history = true;
  check(set_forward(while_plan->root, while_constant_op,
                    count_invariant_first),
        "find while invariant callback");
  invariant_first_calls = 0;
  Executor while_executor(outer(while_plan));
  const Evaluation while_result_value = evaluate_invariant(while_executor, 0);
  close(while_result_value.value, 2, "while invariant result");
  check(invariant_first_calls == 1,
        "while body reuses its inactive invariant result");
}

static Evaluation evaluate(Executor& executor, double theta, double beta) {
  executor.params_data()[0] = theta;
  executor.params_data()[1] = beta;
  Evaluation result;
  result.value = executor.gradient(result.gradient);
  return result;
}

static void dynamic_history_concurrency_tests() {
  auto reference_plan = recurrence(64);
  Executor reference(outer(reference_plan));
  const Evaluation expected_a = evaluate(reference, .1, .7);
  const Evaluation expected_b = evaluate(reference, -.2, .3);

  auto dynamic_plan = recurrence(64);
  dynamic_plan->dynamic_history = true;
  const Graph graph = outer(dynamic_plan);
  Executor first(graph), second(graph);
  std::atomic<bool> start{false};
  std::atomic<bool> correct{true};
  const auto run = [&](Executor& executor, double theta, double beta,
                       const Evaluation& expected) {
    while (!start.load(std::memory_order_acquire)) std::this_thread::yield();
    for (int repetition = 0; repetition < 16; ++repetition) {
      const Evaluation actual = evaluate(executor, theta, beta);
      if (!near(actual.value, expected.value) ||
          !near(actual.gradient[0], expected.gradient[0]) ||
          !near(actual.gradient[1], expected.gradient[1]))
        correct.store(false, std::memory_order_relaxed);
    }
  };
  std::thread a(run, std::ref(first), .1, .7, std::cref(expected_a));
  std::thread b(run, std::ref(second), -.2, .3, std::cref(expected_b));
  start.store(true, std::memory_order_release);
  a.join();
  b.join();
  check(correct.load(std::memory_order_relaxed),
        "dynamic history is isolated across concurrent executors");
}

static std::atomic<bool> fail_forward{false};
static std::atomic<bool> fail_backward{false};

static void failure_test_forward(KernelCtx& context) {
  if (fail_forward.exchange(false))
    throw std::runtime_error("injected structured forward failure");
  find_kernel(OP_TANHV)->forward(context);
}

static void failure_test_backward(KernelCtx& context) {
  if (fail_backward.exchange(false))
    throw std::runtime_error("injected structured backward failure");
  find_kernel(OP_TANHV)->backward(context);
}

static bool install_failure_callbacks(StructuredLoop& plan, Node& node) {
  if (node.kind == Node::KernelCall &&
      plan.body.ops[static_cast<size_t>(node.op)].opcode == OP_TANHV) {
    node.forward = failure_test_forward;
    node.backward = failure_test_backward;
    return true;
  }
  for (auto& child : node.children)
    if (install_failure_callbacks(plan, child)) return true;
  return false;
}

static void dynamic_history_failure_tests() {
  auto plan = recurrence(8);
  plan->dynamic_history = true;
  check(install_failure_callbacks(*plan, plan->root),
        "failure test finds a native callback");
  Executor executor(outer(plan));
  executor.params_data()[0] = .1;
  executor.params_data()[1] = .7;
  double gradient[2];

  fail_forward.store(true);
  bool forward_threw = false;
  try {
    (void)executor.gradient(gradient);
  } catch (const std::runtime_error& error) {
    forward_threw =
        std::string(error.what()) == "injected structured forward failure";
  }
  check(forward_threw, "dynamic history preserves a forward exception");

  fail_backward.store(true);
  bool backward_threw = false;
  try {
    (void)executor.gradient(gradient);
  } catch (const std::runtime_error& error) {
    backward_threw =
        std::string(error.what()) == "injected structured backward failure";
  }
  check(backward_threw, "dynamic history preserves a backward exception");

  auto reference_plan = recurrence(8);
  Executor reference(outer(reference_plan));
  const Evaluation expected = evaluate(reference, .1, .7);
  const Evaluation actual = evaluate(executor, .1, .7);
  close(actual.value, expected.value, "dynamic history retry value");
  close(actual.gradient[0], expected.gradient[0],
        "dynamic history retry theta gradient");
  close(actual.gradient[1], expected.gradient[1],
        "dynamic history retry beta gradient");
}

static void automatic_policy_tests() {
  const auto small_auto = compile_fixture("structured_counted", 4, Mode::Auto);
  const auto small_off = compile_fixture("structured_counted", 4, Mode::Off);
  check(retained(small_auto) == nullptr,
        "small scalar loop stays on the legacy path by default");
  check(same_graph_shape(small_auto.graph, small_off.graph),
        "small automatic graph matches the legacy graph");
  compare_gradients(small_auto, small_off, {{.1, .7}},
                    "small auto target parity", "small auto gradient parity");

  // A repeated outer loop containing runtime control is the structural class
  // that motivated ctsem support. The selector sees the shape and trip count;
  // it does not know this fixture's name.
  const auto hazard_auto = compile_fixture("structured_nested", 32, Mode::Auto);
  const auto hazard_off = compile_fixture("structured_nested", 32, Mode::Off);
  const StructuredLoop* hazard_plan = retained(hazard_auto);
  check(hazard_plan != nullptr,
        "outer repeated nested-control hazard selects OP_LOOP");
  if (hazard_plan) check_native_only(*hazard_plan, "auto body is native-only");
  compare_gradients(hazard_auto, hazard_off, {{.1, .7}, {-.2, .3}},
                    "auto hazard target parity", "auto hazard gradient parity");

  std::string renamed = fixture_mir("structured_nested");
  size_t position = 0;
  while ((position = renamed.find("state", position)) != std::string::npos) {
    renamed.replace(position, 5, "accumulation");
    position += 12;
  }
  test_unsetenv("STANLI_STRUCTURED_LOOPS");
  DataMap renamed_data;
  renamed_data.set_int("N", 32);
  const auto renamed_auto = compile_model(renamed, renamed_data);
  check(retained(renamed_auto) != nullptr,
        "alpha-renaming preserves automatic structural selection");

  // The same structural hazard can contain a body outside native coverage.
  // Force proves that the candidate is unsupported; auto must discard that
  // isolated trial and produce the exact legacy graph rather than publishing
  // a partial OP_LOOP.
  bool force_refused = false;
  try {
    (void)compile_fixture("structured_auto_refusal", 32, Mode::Force);
  } catch (const CompileError&) {
    force_refused = true;
  }
  check(force_refused, "unsupported native candidate is identified");
  const auto refused_auto =
      compile_fixture("structured_auto_refusal", 32, Mode::Auto);
  const auto refused_off =
      compile_fixture("structured_auto_refusal", 32, Mode::Off);
  check(retained(refused_auto) == nullptr,
        "unsupported automatic candidate falls back to legacy");
  check(same_graph_shape(refused_auto.graph, refused_off.graph),
        "refused candidate leaves no partial graph state");
  compare_gradients(refused_auto, refused_off, {{.25}, {-1.5}},
                    "refusal target parity", "refusal gradient parity");
}

int main() {
  test_unsetenv("STANLI_STRUCTURED_LOOP_DIAGNOSTICS");
  runtime_trip_tests();
  forced_control_tests();
  dynamic_history_tests();
  loop_invariant_reuse_tests();
  dynamic_history_concurrency_tests();
  dynamic_history_failure_tests();
  automatic_policy_tests();
  test_unsetenv("STANLI_STRUCTURED_LOOPS");
  test_unsetenv("STANLI_STRUCTURED_HISTORY_BYTES");
  test_unsetenv("STANLI_NO_STRUCTURED_INVARIANT_REUSE");
  if (failures == 0) std::printf("test_structured_loop_production OK\n");
  return failures != 0;
}
