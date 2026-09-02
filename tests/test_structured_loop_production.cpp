#include "env_helpers.hpp"

#include <stanli/compile.hpp>
#include <stanli/graph.hpp>
#include <stanli/optable.hpp>
#include <stanli/structured_loop.hpp>
#include <stan/math.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <functional>
#include <limits>
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

static bool set_backward(Node& node, int op, void (*backward)(KernelCtx&)) {
  if (node.kind == Node::KernelCall && node.op == op) {
    node.backward = backward;
    return true;
  }
  for (auto& child : node.children)
    if (set_backward(child, op, backward)) return true;
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

static void compact_import_reference_tests() {
  auto plan = std::make_shared<StructuredLoop>();
  const int left = plan->body.add_slot(1, false);
  const int right = plan->body.add_slot(1, false);
  const int repeated = plan->body.add_slot(1, false);
  const int product = plan->body.add_slot(1, false);
  const int result = plan->body.add_slot(1, false);
  // Import ordinals deliberately differ from slot order. Two imports also
  // name the same nonzero outer offset, so reverse must accumulate through
  // distinct compact Ref entries into one graph adjoint cell.
  plan->imports = {{repeated, 0, 1}, {left, 1, 2}, {right, 0, 1}};
  plan->root = sequence({call(*plan, OP_MUL, {left, right}, product),
                         call(*plan, OP_ADD, {product, repeated}, result)});
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;

  Graph graph;
  graph.add_slot(3, true);
  graph.add_slot(3, true);
  const int output = graph.add_slot(1, false);
  const int op = graph.add_op(OP_LOOP, {0, 1}, output);
  graph.ops[op].udata = plan.get();
  graph.udata_pool.push_back(std::move(plan));
  graph.result_slot = output;

  Executor executor(graph);
  const double point[] = {1, 2, 3, 4, 5, 6};
  std::copy(std::begin(point), std::end(point), executor.params_data());
  double gradient[6] = {};
  close(executor.gradient(gradient), 14,
        "compact import ordinals preserve value");
  const double expected[] = {0, 7, 0, 0, 0, 2};
  for (size_t i = 0; i < std::size(expected); ++i)
    close(gradient[i], expected[i],
          "compact import ordinals preserve outer gradient");
}

static void direct_index_kernel_tests() {
  const Kernel* dynamic_index = find_kernel(OP_INDEX_DYNAMIC);
  check(dynamic_index && dynamic_index->forward && dynamic_index->backward,
        "dynamic index kernel registered");
  if (!dynamic_index || !dynamic_index->forward || !dynamic_index->backward)
    return;

  DynamicIndexSpec packed_spec;
  packed_spec.matrix_leaf = true;
  packed_spec.axes = {
      {DynamicIndexSpec::Axis::Single, 2, 1, 1, 0},
      {DynamicIndexSpec::Axis::Range, 3, 2, 3, 1},
  };
  packed_spec.axes[1].count_input_offset = 2;
  packed_spec.selected_size = 3;
  double base[] = {1, 2, 3, 4, 5, 6};
  double selectors[] = {2, 1, 3};
  double packed_output[3] = {};
  double packed_base_adj[6] = {};
  double seed[] = {1, 2, 3};
  KernelCtx packed{};
  packed.n_in = 2;
  packed.in[0] = {base, 6};
  packed.in[1] = {selectors, 3};
  packed.in_adj[0] = {packed_base_adj, 6};
  packed.in_adj[1] = {nullptr, 3};
  packed.out = {packed_output, 3};
  packed.out_adj_vec = {seed, 3};
  packed.udata = &packed_spec;
  dynamic_index->forward(packed);
  dynamic_index->backward(packed);

  DynamicIndexSpec direct_spec = packed_spec;
  direct_spec.input_count = 4;
  direct_spec.axes[0].selector_input = 1;
  direct_spec.axes[0].input_offset = 0;
  direct_spec.axes[1].selector_input = 2;
  direct_spec.axes[1].input_offset = 0;
  direct_spec.axes[1].count_input = 3;
  direct_spec.axes[1].count_input_offset = 0;
  double row = 2, lower = 1, upper = 3;
  double direct_output[3] = {};
  double direct_base_adj[6] = {};
  KernelCtx direct{};
  direct.n_in = 4;
  direct.in[0] = {base, 6};
  direct.in[1] = {&row, 1};
  direct.in[2] = {&lower, 1};
  direct.in[3] = {&upper, 1};
  direct.in_adj[0] = {direct_base_adj, 6};
  direct.in_adj[1] = {nullptr, 1};
  direct.in_adj[2] = {nullptr, 1};
  direct.in_adj[3] = {nullptr, 1};
  direct.out = {direct_output, 3};
  direct.out_adj_vec = {seed, 3};
  direct.udata = &direct_spec;
  dynamic_index->forward(direct);
  dynamic_index->backward(direct);
  check(std::memcmp(packed_output, direct_output, sizeof(packed_output)) == 0,
        "direct index matches packed values bitwise");
  check(std::memcmp(packed_base_adj, direct_base_adj,
                    sizeof(packed_base_adj)) == 0,
        "direct index matches packed adjoints bitwise");

  upper = 0;
  dynamic_index->forward(direct);
  check(direct_output[0] == 0 && direct_output[1] == 0 && direct_output[2] == 0,
        "direct dynamic range supports zero selected values");
  upper = 4;
  lower = 4;
  try {
    dynamic_index->forward(direct);
    check(false, "direct index validates selectors");
  } catch (const std::out_of_range&) {
  }
  lower = 1;
  direct.n_in = 3;
  try {
    dynamic_index->forward(direct);
    check(false, "direct index validates input arity");
  } catch (const std::logic_error&) {
  }
  direct.n_in = 4;
  lower = 1;
  upper = 3;

  DynamicIndexSpec invalid_route = direct_spec;
  invalid_route.axes[0].selector_input = 0;
  direct.udata = &invalid_route;
  try {
    dynamic_index->forward(direct);
    check(false, "direct index rejects the base as a selector descriptor");
  } catch (const std::logic_error&) {
  }
  invalid_route = direct_spec;
  invalid_route.axes[1].count_input = 4;
  direct.udata = &invalid_route;
  try {
    dynamic_index->forward(direct);
    check(false, "direct index rejects an out-of-range count descriptor");
  } catch (const std::logic_error&) {
  }
  direct.udata = &direct_spec;

  DynamicIndexSpec extent_spec;
  extent_spec.input_count = 2;
  extent_spec.axes = {{DynamicIndexSpec::Axis::All, 4, 1, 4, 0}};
  extent_spec.axes[0].extent_input = 1;
  extent_spec.axes[0].extent_input_offset = 0;
  extent_spec.axes[0].count_input = 1;
  extent_spec.axes[0].count_input_offset = 0;
  extent_spec.selected_size = 4;
  double logical_extent = 2;
  double extent_output[4] = {-1, -1, -1, -1};
  KernelCtx extent{};
  extent.n_in = 2;
  extent.in[0] = {base, 4};
  extent.in[1] = {&logical_extent, 1};
  extent.out = {extent_output, 4};
  extent.udata = &extent_spec;
  dynamic_index->forward(extent);
  check(extent_output[0] == 1 && extent_output[1] == 2 &&
            extent_output[2] == 0 && extent_output[3] == 0,
        "direct logical extent preserves output capacity and zero fill");
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

  // This fixture feeds the counted iterator directly to an active multiply.
  // Reverse must therefore recover every historical iterator value from the
  // retained input handles rather than from one mutable loop cell.
  test_setenv("STANLI_STRUCTURED_HISTORY_BYTES", "1");
  const auto counted_dynamic =
      compile_fixture("structured_counted", 4, Mode::Force);
  test_unsetenv("STANLI_STRUCTURED_HISTORY_BYTES");
  const auto counted_legacy =
      compile_fixture("structured_counted", 4, Mode::Off);
  check(retained(counted_dynamic) && retained(counted_dynamic)->dynamic_history,
        "counted iterator test exercises dynamic history");
  compare_gradients(counted_dynamic, counted_legacy,
                    {{.1, .7}, {-.2, .3}, {0, .5}},
                    "dynamic counted-iterator target parity",
                    "dynamic counted-iterator gradient parity");

  // Exercise iterator-driven nested branches, break, and continue on the
  // reached-frame tape as well as on the ordinary small fixed-history path.
  test_setenv("STANLI_STRUCTURED_HISTORY_BYTES", "1");
  const auto exits_dynamic =
      compile_fixture("structured_exits", 6, Mode::Force);
  test_unsetenv("STANLI_STRUCTURED_HISTORY_BYTES");
  const auto exits_legacy = compile_fixture("structured_exits", 6, Mode::Off);
  check(retained(exits_dynamic) && retained(exits_dynamic)->dynamic_history,
        "iterator exit test exercises dynamic history");
  compare_gradients(exits_dynamic, exits_legacy, {{.1, .7}, {-.2, .3}},
                    "dynamic iterator-exit target parity",
                    "dynamic iterator-exit gradient parity");
}

struct Evaluation {
  double value = 0;
  double gradient[2] = {0, 0};
};

static Evaluation evaluate(Executor& executor, double theta, double beta);

static std::vector<double> iterator_forward_values;
static std::vector<double> iterator_reverse_values;
static std::vector<std::array<double, 3>> update_reverse_values;
static std::vector<std::array<double, 6>> range_update_reverse_values;
static std::vector<const double*> range_update_forward_addresses;
static bool iterator_throw_once = false;
static int ordinary_update_forward_calls = 0;
static int ordinary_update_backward_calls = 0;

static void trace_iterator_forward(KernelCtx& context) {
  iterator_forward_values.push_back(context.in[1].data[0]);
  if (iterator_throw_once && context.in[1].data[0] == 0) {
    iterator_throw_once = false;
    throw std::runtime_error("injected iterator callback failure");
  }
  find_kernel(OP_MUL)->forward(context);
}

static void trace_iterator_backward(KernelCtx& context) {
  iterator_reverse_values.push_back(context.in[1].data[0]);
  find_kernel(OP_MUL)->backward(context);
}

static void trace_update_backward(KernelCtx& context) {
  update_reverse_values.push_back(
      {context.in[0].data[0], context.in[0].data[1], context.in[0].data[2]});
  find_kernel(OP_SET_INDEX_DYNAMIC)->backward(context);
}

static void retained_update_backward(KernelCtx& context) {
  find_kernel(OP_SET_INDEX_DYNAMIC)->backward(context);
}

static void ordinary_update_forward(KernelCtx& context) {
  ++ordinary_update_forward_calls;
  find_kernel(OP_SET_INDEX_DYNAMIC)->forward(context);
}

static void ordinary_update_backward(KernelCtx& context) {
  ++ordinary_update_backward_calls;
  find_kernel(OP_SET_INDEX_DYNAMIC)->backward(context);
}

static void trace_range_sum_backward(KernelCtx& context) {
  std::array<double, 6> values{};
  std::copy_n(context.in[0].data, values.size(), values.data());
  range_update_reverse_values.push_back(values);
  find_kernel(OP_SUM_VEC)->backward(context);
}

static void trace_range_sum_forward(KernelCtx& context) {
  range_update_forward_addresses.push_back(context.in[0].data);
  find_kernel(OP_SUM_VEC)->forward(context);
}

static std::shared_ptr<StructuredLoop> iterator_history_plan() {
  auto plan = std::make_shared<StructuredLoop>();
  const int theta = plan->body.add_slot(1, false);
  const int lower = scalar(*plan, -1);
  const int upper = scalar(*plan, 1);
  const int iterator = plan->body.add_slot(1, false);
  const int zero = scalar(*plan, 0);
  const int result = plan->body.add_slot(1, false);
  const int term = plan->body.add_slot(1, false);
  const int next = plan->body.add_slot(1, false);
  plan->imports = {{theta, 0, 0}};
  Node multiply = call(*plan, OP_MUL, {theta, iterator}, term);
  const int multiply_op = multiply.op;
  plan->root =
      sequence({alias(result, zero),
                counted(lower, upper, iterator, 3,
                        sequence({std::move(multiply),
                                  call(*plan, OP_ADD, {result, term}, next),
                                  alias(result, next)}))});
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  check(set_forward(plan->root, multiply_op, trace_iterator_forward),
        "find iterator trace forward callback");
  check(set_backward(plan->root, multiply_op, trace_iterator_backward),
        "find iterator trace backward callback");
  return plan;
}

static std::shared_ptr<StructuredLoop> iterator_escape_plan(
    int32_t lower_value, int32_t upper_value) {
  auto plan = std::make_shared<StructuredLoop>();
  const int lower = scalar(*plan, lower_value);
  const int upper = scalar(*plan, upper_value);
  const int iterator = plan->body.add_slot(1, false);
  const int initial = scalar(*plan, 7);
  const int result = plan->body.add_slot(1, false);
  const int64_t capacity =
      upper_value >= lower_value
          ? static_cast<int64_t>(upper_value) - lower_value + 1
          : 0;
  plan->root = sequence(
      {alias(result, initial),
       counted(lower, upper, iterator, capacity, alias(result, iterator))});
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  return plan;
}

static std::shared_ptr<StructuredLoop> iterator_target_plan() {
  auto plan = std::make_shared<StructuredLoop>();
  const int lower = scalar(*plan, 1);
  const int upper = scalar(*plan, 3);
  const int iterator = plan->body.add_slot(1, false);
  Node target;
  target.kind = Node::Target;
  target.src = iterator;
  plan->root = counted(lower, upper, iterator, 3, std::move(target));
  plan->has_target = true;
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  return plan;
}

static std::shared_ptr<StructuredLoop> nested_iterator_plan() {
  auto plan = std::make_shared<StructuredLoop>();
  const int theta = plan->body.add_slot(1, false);
  const int zero = scalar(*plan, 0);
  const int two = scalar(*plan, 2);
  const int outer_iterator = plan->body.add_slot(1, false);
  const int inner_iterator = plan->body.add_slot(1, false);
  const int sum = plan->body.add_slot(1, false);
  const int term = plan->body.add_slot(1, false);
  const int result = plan->body.add_slot(1, false);
  const int next = plan->body.add_slot(1, false);
  plan->imports = {{theta, 0, 0}};
  plan->root = sequence(
      {alias(result, zero),
       counted(zero, two, outer_iterator, 3,
               counted(zero, outer_iterator, inner_iterator, 3,
                       sequence({call(*plan, OP_ADD,
                                      {outer_iterator, inner_iterator}, sum),
                                 call(*plan, OP_MUL, {theta, sum}, term),
                                 call(*plan, OP_ADD, {result, term}, next),
                                 alias(result, next)})))});
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  return plan;
}

static std::shared_ptr<StructuredLoop> iterator_update_plan(
    void (*backward)(KernelCtx&) = trace_update_backward,
    void (*forward)(KernelCtx&) = nullptr) {
  auto plan = std::make_shared<StructuredLoop>();
  const int theta = plan->body.add_slot(1, false);
  const int lower = scalar(*plan, 1);
  const int upper = scalar(*plan, 3);
  const int iterator = plan->body.add_slot(1, false);
  const int base = plan->body.add_slot(3, false);
  plan->fills.push_back({base, {10, 20, 30}});
  const int current = plan->body.add_slot(3, false);
  const int rhs = plan->body.add_slot(1, false);
  const int updated = plan->body.add_slot(3, false);
  const int result = plan->body.add_slot(1, false);
  plan->imports = {{theta, 0, 0}};

  auto spec = std::make_shared<DynamicIndexSpec>();
  spec->axes = {{DynamicIndexSpec::Axis::Single, 3, 1, 1, 0}};
  spec->selected_size = 1;
  Node update =
      call(*plan, OP_SET_INDEX_DYNAMIC, {current, iterator, rhs}, updated);
  const int update_op = update.op;
  plan->body.ops[static_cast<size_t>(update.op)].udata = spec.get();
  plan->body.udata_pool.push_back(spec);
  plan->root =
      sequence({alias(current, base),
                counted(lower, upper, iterator, 3,
                        sequence({call(*plan, OP_MUL, {theta, iterator}, rhs),
                                  std::move(update), alias(current, updated)})),
                call(*plan, OP_SUM_VEC, {current}, result)});
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  check(plan->compact_update_sites == 1,
        "iterator update plan selects compact history");
  if (backward)
    check(set_backward(plan->root, update_op, backward),
          "find compact-update replacement backward callback");
  if (forward)
    check(set_forward(plan->root, update_op, forward),
          "find compact-update replacement forward callback");
  return plan;
}

static std::shared_ptr<StructuredLoop> range_update_plan(bool force_ordinary) {
  auto plan = std::make_shared<StructuredLoop>();
  const int base = plan->body.add_slot(6, false);
  const int theta = plan->body.add_slot(1, false);
  const int beta = plan->body.add_slot(1, false);
  const int lower = scalar(*plan, 1);
  const int upper = scalar(*plan, 3);
  const int iterator = plan->body.add_slot(1, false);
  const int selector = scalar(*plan, 1);
  const int weights = plan->body.add_slot(6, false);
  plan->fills.push_back({weights, {1, 2, 4, 8, 16, 32}});
  const int current = plan->body.add_slot(6, false);
  const int theta_term = plan->body.add_slot(1, false);
  const int beta_term = plan->body.add_slot(1, false);
  const int rhs = plan->body.add_slot(2, false);
  const int updated = plan->body.add_slot(6, false);
  const int zero_selector = plan->body.add_slot(2, false);
  plan->fills.push_back({zero_selector, {1, 0}});
  const int zero_updated = plan->body.add_slot(6, false);
  const int observed = plan->body.add_slot(1, false);
  const int result = plan->body.add_slot(1, false);
  plan->imports = {{base, 0, 0}, {theta, 1, 0}, {beta, 2, 0}};

  auto spec = std::make_shared<DynamicIndexSpec>();
  spec->axes = {{DynamicIndexSpec::Axis::Range, 6, 1, 2, 0}};
  spec->selected_size = 2;
  Node update =
      call(*plan, OP_SET_INDEX_DYNAMIC, {current, selector, rhs}, updated);
  const int update_op = update.op;
  plan->body.ops[static_cast<size_t>(update_op)].udata = spec.get();
  plan->body.udata_pool.push_back(spec);
  auto zero_spec = std::make_shared<DynamicIndexSpec>();
  zero_spec->axes = {{DynamicIndexSpec::Axis::Range, 6, 1, 2, 0}};
  zero_spec->axes[0].count_input_offset = 1;
  zero_spec->selected_size = 2;
  Node zero_update = call(*plan, OP_SET_INDEX_DYNAMIC,
                          {current, zero_selector, rhs}, zero_updated);
  const int zero_update_op = zero_update.op;
  plan->body.ops[static_cast<size_t>(zero_update_op)].udata = zero_spec.get();
  plan->body.udata_pool.push_back(zero_spec);
  Node observe = call(*plan, OP_SUM_VEC, {current}, observed);
  const int observe_op = observe.op;
  plan->root = sequence(
      {alias(current, base),
       counted(lower, upper, iterator, 3,
               sequence({call(*plan, OP_MUL, {theta, iterator}, theta_term),
                         call(*plan, OP_MUL, {beta, iterator}, beta_term),
                         call(*plan, OP_CONCAT2, {theta_term, beta_term}, rhs),
                         std::move(update), alias(current, updated),
                         std::move(observe)})),
       std::move(zero_update), alias(current, zero_updated),
       call(*plan, OP_DOT, {current, weights}, result)});
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  check(plan->compact_update_sites == 2,
        "ordered range update selects compact history");
  check(set_backward(plan->root, observe_op, trace_range_sum_backward),
        "find ordered range observer");
  check(set_forward(plan->root, observe_op, trace_range_sum_forward),
        "find ordered range forward observer");
  if (force_ordinary) {
    check(set_forward(plan->root, update_op, ordinary_update_forward),
          "find ordered range update forward callback");
    check(set_backward(plan->root, update_op, ordinary_update_backward),
          "find ordered range update backward callback");
    check(set_forward(plan->root, zero_update_op, ordinary_update_forward),
          "find zero range update forward callback");
    check(set_backward(plan->root, zero_update_op, ordinary_update_backward),
          "find zero range update backward callback");
  }
  return plan;
}

static Graph range_update_outer(std::shared_ptr<StructuredLoop> plan) {
  Graph graph;
  graph.add_slot(6, true);
  graph.add_slot(1, true);
  graph.add_slot(1, true);
  const int output = graph.add_slot(1, false);
  const int op = graph.add_op(OP_LOOP, {0, 1, 2}, output);
  graph.ops[op].udata = plan.get();
  graph.udata_pool.push_back(std::move(plan));
  graph.result_slot = output;
  return graph;
}

static std::shared_ptr<StructuredLoop> inactive_range_update_plan(
    bool force_ordinary) {
  auto plan = std::make_shared<StructuredLoop>();
  const int theta = plan->body.add_slot(1, false);
  const int lower = scalar(*plan, 1);
  const int upper = scalar(*plan, 3);
  const int iterator = plan->body.add_slot(1, false);
  const int base = plan->body.add_slot(16, false);
  plan->fills.push_back(
      {base, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}});
  const int current = plan->body.add_slot(16, false);
  const int selector = scalar(*plan, 1);
  const int rhs = plan->body.add_slot(2, false);
  plan->fills.push_back({rhs, {10, 20}});
  const int updated = plan->body.add_slot(16, false);
  const int total = plan->body.add_slot(1, false);
  const int result = plan->body.add_slot(1, false);
  plan->imports = {{theta, 0, 0}};

  auto spec = std::make_shared<DynamicIndexSpec>();
  spec->axes = {{DynamicIndexSpec::Axis::Range, 16, 1, 2, 0}};
  spec->selected_size = 2;
  Node update =
      call(*plan, OP_SET_INDEX_DYNAMIC, {current, selector, rhs}, updated);
  const int update_op = update.op;
  plan->body.ops[static_cast<size_t>(update_op)].udata = spec.get();
  plan->body.udata_pool.push_back(spec);
  plan->root =
      sequence({alias(current, base),
                counted(lower, upper, iterator, 3,
                        sequence({std::move(update), alias(current, updated)})),
                call(*plan, OP_SUM_VEC, {current}, total),
                call(*plan, OP_MUL, {theta, total}, result)});
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  check(plan->compact_update_sites == 1,
        "inactive ordered range selects compact history");
  if (force_ordinary) {
    check(set_forward(plan->root, update_op, ordinary_update_forward),
          "find inactive range update forward callback");
    check(set_backward(plan->root, update_op, ordinary_update_backward),
          "find inactive range update backward callback");
  }
  return plan;
}

static std::shared_ptr<StructuredLoop> aliased_update_plan(
    bool force_ordinary) {
  auto plan = std::make_shared<StructuredLoop>();
  const int theta = plan->body.add_slot(1, false);
  const int one = scalar(*plan, 1);
  const int two = scalar(*plan, 2);
  const int three = scalar(*plan, 3);
  const int base = plan->body.add_slot(3, false);
  plan->fills.push_back({base, {10, 20, 30}});
  const int current = plan->body.add_slot(3, false);
  const int snapshot1 = plan->body.add_slot(3, false);
  const int snapshot2 = plan->body.add_slot(3, false);
  const int rhs1 = plan->body.add_slot(1, false);
  const int rhs2 = plan->body.add_slot(1, false);
  const int rhs3 = plan->body.add_slot(1, false);
  const int updated1_slot = plan->body.add_slot(3, false);
  const int updated2_slot = plan->body.add_slot(3, false);
  const int updated3_slot = plan->body.add_slot(3, false);
  const int observed1 = plan->body.add_slot(1, false);
  const int observed2 = plan->body.add_slot(1, false);
  const int observed3 = plan->body.add_slot(1, false);
  const int snapshot_sum1 = plan->body.add_slot(1, false);
  const int snapshot_sum2 = plan->body.add_slot(1, false);
  const int current_sum = plan->body.add_slot(1, false);
  const int snapshot_total = plan->body.add_slot(1, false);
  const int result = plan->body.add_slot(1, false);
  plan->imports = {{theta, 0, 0}};

  auto spec = std::make_shared<DynamicIndexSpec>();
  spec->axes = {{DynamicIndexSpec::Axis::Single, 3, 1, 1, 0}};
  spec->selected_size = 1;
  Node update1 =
      call(*plan, OP_SET_INDEX_DYNAMIC, {current, one, rhs1}, updated1_slot);
  Node update2 =
      call(*plan, OP_SET_INDEX_DYNAMIC, {current, two, rhs2}, updated2_slot);
  Node update3 =
      call(*plan, OP_SET_INDEX_DYNAMIC, {current, three, rhs3}, updated3_slot);
  const int update_ops[] = {update1.op, update2.op, update3.op};
  for (int op : update_ops)
    plan->body.ops[static_cast<size_t>(op)].udata = spec.get();
  plan->body.udata_pool.push_back(spec);
  Node observe1 = call(*plan, OP_SUM_VEC, {current}, observed1);
  Node observe2 = call(*plan, OP_SUM_VEC, {current}, observed2);
  Node observe3 = call(*plan, OP_SUM_VEC, {current}, observed3);
  const int observer_ops[] = {observe1.op, observe2.op, observe3.op};
  plan->root = sequence(
      {alias(current, base), call(*plan, OP_MUL, {theta, one}, rhs1),
       std::move(update1), alias(current, updated1_slot), std::move(observe1),
       alias(snapshot1, current), alias(snapshot2, current),
       call(*plan, OP_MUL, {theta, two}, rhs2), std::move(update2),
       alias(current, updated2_slot), std::move(observe2),
       call(*plan, OP_MUL, {theta, three}, rhs3), std::move(update3),
       alias(current, updated3_slot), std::move(observe3),
       call(*plan, OP_SUM_VEC, {snapshot1}, snapshot_sum1),
       call(*plan, OP_SUM_VEC, {snapshot2}, snapshot_sum2),
       call(*plan, OP_SUM_VEC, {current}, current_sum),
       call(*plan, OP_ADD, {snapshot_sum1, snapshot_sum2}, snapshot_total),
       call(*plan, OP_ADD, {snapshot_total, current_sum}, result)});
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  check(plan->compact_update_sites == 3,
        "outgoing alias keeps compact update sites eligible");
  for (int op : observer_ops)
    check(set_forward(plan->root, op, trace_range_sum_forward),
          "find aliased update address observer");
  if (force_ordinary) {
    for (int op : update_ops) {
      check(set_forward(plan->root, op, ordinary_update_forward),
            "find aliased update forward callback");
      check(set_backward(plan->root, op, ordinary_update_backward),
            "find aliased update backward callback");
    }
  }
  return plan;
}

static std::shared_ptr<StructuredLoop> loop_aliased_update_plan(
    bool force_ordinary) {
  auto plan = std::make_shared<StructuredLoop>();
  const int theta = plan->body.add_slot(1, false);
  const int lower = scalar(*plan, 1);
  const int upper = scalar(*plan, 3);
  const int iterator = plan->body.add_slot(1, false);
  const int base = plan->body.add_slot(3, false);
  plan->fills.push_back({base, {10, 20, 30}});
  const int current = plan->body.add_slot(3, false);
  const int snapshot = plan->body.add_slot(3, false);
  const int rhs = plan->body.add_slot(1, false);
  const int updated = plan->body.add_slot(3, false);
  const int observed = plan->body.add_slot(1, false);
  const int first = plan->body.add_slot(1, false);
  const int snapshot_sum = plan->body.add_slot(1, false);
  const int current_sum = plan->body.add_slot(1, false);
  const int result = plan->body.add_slot(1, false);
  plan->imports = {{theta, 0, 0}};

  auto spec = std::make_shared<DynamicIndexSpec>();
  spec->axes = {{DynamicIndexSpec::Axis::Single, 3, 1, 1, 0}};
  spec->selected_size = 1;
  Node update =
      call(*plan, OP_SET_INDEX_DYNAMIC, {current, iterator, rhs}, updated);
  const int update_op = update.op;
  plan->body.ops[static_cast<size_t>(update_op)].udata = spec.get();
  plan->body.udata_pool.push_back(spec);
  Node observe = call(*plan, OP_SUM_VEC, {current}, observed);
  const int observe_op = observe.op;
  Node is_first = call(*plan, OP_COMPARE, {iterator, lower}, first);
  plan->body.ops[static_cast<size_t>(is_first.op)].variant = 4;
  plan->root = sequence(
      {alias(current, base),
       counted(lower, upper, iterator, 3,
               sequence({call(*plan, OP_MUL, {theta, iterator}, rhs),
                         std::move(update), alias(current, updated),
                         std::move(observe), std::move(is_first),
                         branch(first, alias(snapshot, current), sequence({}))})),
       call(*plan, OP_SUM_VEC, {snapshot}, snapshot_sum),
       call(*plan, OP_SUM_VEC, {current}, current_sum),
       call(*plan, OP_ADD, {snapshot_sum, current_sum}, result)});
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  check(plan->compact_update_sites == 1,
        "loop-backedge alias keeps compact update site eligible");
  check(set_forward(plan->root, observe_op, trace_range_sum_forward),
        "find loop-backedge update address observer");
  if (force_ordinary) {
    check(set_forward(plan->root, update_op, ordinary_update_forward),
          "find loop-backedge update forward callback");
    check(set_backward(plan->root, update_op, ordinary_update_backward),
          "find loop-backedge update backward callback");
  }
  return plan;
}

static void compact_iterator_history_tests() {
  iterator_forward_values.clear();
  iterator_reverse_values.clear();
  Executor history(outer(iterator_history_plan()));
  const Evaluation traced = evaluate(history, .25, 0);
  close(traced.value, 0, "inline iterator history value");
  close(traced.gradient[0], 0, "inline iterator history theta gradient");
  check(iterator_forward_values == std::vector<double>({-1, 0, 1}),
        "inline iterator forward values are exact");
  check(iterator_reverse_values == std::vector<double>({1, 0, -1}),
        "inline iterator reverse values preserve history");

  Executor retry(outer(iterator_history_plan()));
  iterator_throw_once = true;
  bool threw = false;
  try {
    (void)evaluate(retry, .25, 0);
  } catch (const std::runtime_error& error) {
    threw = std::string(error.what()) == "injected iterator callback failure";
  }
  check(threw, "inline iterator preserves callback exception");
  iterator_forward_values.clear();
  iterator_reverse_values.clear();
  const Evaluation retried = evaluate(retry, .25, 0);
  close(retried.value, traced.value, "inline iterator retry value");
  close(retried.gradient[0], traced.gradient[0],
        "inline iterator retry theta gradient");
  check(iterator_forward_values == std::vector<double>({-1, 0, 1}) &&
            iterator_reverse_values == std::vector<double>({1, 0, -1}),
        "inline iterator retry rebuilds historical values");

  for (const auto& bounds : std::vector<std::pair<int32_t, int32_t>>{
           {std::numeric_limits<int32_t>::min(),
            std::numeric_limits<int32_t>::min() + 1},
           {std::numeric_limits<int32_t>::max() - 1,
            std::numeric_limits<int32_t>::max()},
           {1, 0}}) {
    Executor escaped(outer(iterator_escape_plan(bounds.first, bounds.second)));
    const Evaluation result = evaluate(escaped, 0, 0);
    const double expected = bounds.second >= bounds.first ? bounds.second : 7;
    close(result.value, expected, "inline iterator boundary escape value");
    close(result.gradient[0], 0,
          "inline iterator boundary escape theta gradient");
    close(result.gradient[1], 0,
          "inline iterator boundary escape beta gradient");
  }

  Executor target(outer(iterator_target_plan()));
  const Evaluation targeted = evaluate(target, 0, 0);
  close(targeted.value, 6, "inline iterator target keeps reached values");
  close(targeted.gradient[0], 0, "inline iterator target theta gradient");
  close(targeted.gradient[1], 0, "inline iterator target beta gradient");

  Executor nested(outer(nested_iterator_plan()));
  const Evaluation nested_result = evaluate(nested, .25, 0);
  close(nested_result.value, 3, "nested inline iterator value");
  close(nested_result.gradient[0], 12, "nested inline iterator theta gradient");
  close(nested_result.gradient[1], 0, "nested inline iterator beta gradient");

  update_reverse_values.clear();
  Executor update(outer(iterator_update_plan()));
  const Evaluation updated = evaluate(update, .25, 0);
  close(updated.value, 1.5, "inline iterator compact-update value");
  close(updated.gradient[0], 6,
        "inline iterator compact-update theta gradient");
  close(updated.gradient[1], 0, "inline iterator compact-update beta gradient");
  check(update_reverse_values ==
            std::vector<std::array<double, 3>>(
                {{{.25, .5, 30}}, {{.25, 20, 30}}, {{10, 20, 30}}}),
        "custom update reverse sees ordinary historical inputs");

  Executor frame_free(outer(iterator_update_plan(nullptr)));
  Executor retained(outer(iterator_update_plan(retained_update_backward)));
  const Evaluation compact = evaluate(frame_free, .25, 0);
  const Evaluation reference = evaluate(retained, .25, 0);
  check(std::memcmp(&compact, &reference, sizeof(Evaluation)) == 0,
        "frame-free compact reverse has bitwise ordinary-path parity");
  const Evaluation repeated = evaluate(frame_free, .25, 0);
  check(std::memcmp(&compact, &repeated, sizeof(Evaluation)) == 0,
        "frame-free compact reverse resets between evaluations");
  ordinary_update_forward_calls = 0;
  Executor custom_forward(
      outer(iterator_update_plan(nullptr, ordinary_update_forward)));
  const Evaluation custom_forward_result = evaluate(custom_forward, .25, 0);
  check(ordinary_update_forward_calls == 3 &&
            std::memcmp(&compact, &custom_forward_result, sizeof(Evaluation)) ==
                0,
        "custom update forward callback forces the ordinary path");

  struct RangeEvaluation {
    double value = 0;
    double gradient[8] = {};
    std::vector<std::array<double, 6>> reverse_values;
    std::vector<const double*> forward_addresses;
  };
  const auto run_range = [](bool ordinary) {
    range_update_reverse_values.clear();
    range_update_forward_addresses.clear();
    Executor executor(range_update_outer(range_update_plan(ordinary)));
    const double point[] = {10, 20, 30, 40, 50, 60, .25, -.5};
    std::copy(std::begin(point), std::end(point), executor.params_data());
    RangeEvaluation result;
    result.value = executor.gradient(result.gradient);
    result.reverse_values = range_update_reverse_values;
    result.forward_addresses = range_update_forward_addresses;
    return result;
  };
  ordinary_update_forward_calls = ordinary_update_backward_calls = 0;
  range_update_forward_addresses.clear();
  const RangeEvaluation delta = run_range(false);
  const RangeEvaluation ordinary = run_range(true);
  check(std::memcmp(&delta.value, &ordinary.value, sizeof(double)) == 0 &&
            std::memcmp(delta.gradient, ordinary.gradient,
                        sizeof(delta.gradient)) == 0,
        "ordered range delta has bitwise ordinary-path parity");
  check(
      ordinary_update_forward_calls == 4 && ordinary_update_backward_calls == 4,
      "custom ordered range callbacks force the ordinary path");
  close(delta.value, 3157.75, "ordered range delta result");
  const double expected_gradient[] = {0, 0, 4, 8, 16, 32, 3, 6};
  for (size_t i = 0; i < std::size(expected_gradient); ++i)
    close(delta.gradient[i], expected_gradient[i],
          "ordered range delta gradient");
  const std::vector<std::array<double, 6>> expected_reverse = {
      {{.75, -1.5, 30, 40, 50, 60}},
      {{.5, -1, 30, 40, 50, 60}},
      {{.25, -.5, 30, 40, 50, 60}},
  };
  check(delta.reverse_values == expected_reverse &&
            ordinary.reverse_values == expected_reverse,
        "ordered range delta restores historical primals in LIFO order");
  check(delta.forward_addresses.size() == 3 &&
            delta.forward_addresses[0] == delta.forward_addresses[1] &&
            delta.forward_addresses[1] == delta.forward_addresses[2],
        "ordered range delta reuses one anchored primal buffer");
  check(ordinary.forward_addresses.size() == 3 &&
            ordinary.forward_addresses[0] != ordinary.forward_addresses[1] &&
            ordinary.forward_addresses[1] != ordinary.forward_addresses[2],
        "ordinary range fallback keeps distinct output buffers");

  ordinary_update_forward_calls = ordinary_update_backward_calls = 0;
  Executor inactive_delta(outer(inactive_range_update_plan(false)));
  Executor inactive_ordinary(outer(inactive_range_update_plan(true)));
  const Evaluation inactive_compact = evaluate(inactive_delta, .25, 0);
  const Evaluation inactive_reference = evaluate(inactive_ordinary, .25, 0);
  check(std::memcmp(&inactive_compact, &inactive_reference,
                    sizeof(Evaluation)) == 0 &&
            ordinary_update_forward_calls == 3 &&
            ordinary_update_backward_calls == 0,
        "inactive range delta has bitwise ordinary-path parity");
  close(inactive_compact.value, 40.75, "inactive range delta result");
  close(inactive_compact.gradient[0], 163,
        "inactive range delta outer gradient");

  ordinary_update_forward_calls = ordinary_update_backward_calls = 0;
  Executor aliased_compact(outer(aliased_update_plan(false)));
  Executor aliased_ordinary(outer(aliased_update_plan(true)));
  range_update_forward_addresses.clear();
  const Evaluation aliased = evaluate(aliased_compact, .25, 0);
  const auto aliased_compact_addresses = range_update_forward_addresses;
  range_update_forward_addresses.clear();
  const Evaluation aliased_reference = evaluate(aliased_ordinary, .25, 0);
  const auto aliased_ordinary_addresses = range_update_forward_addresses;
  check(std::memcmp(&aliased, &aliased_reference, sizeof(Evaluation)) == 0 &&
            ordinary_update_forward_calls == 3 &&
            ordinary_update_backward_calls == 3,
        "outgoing alias invalidation has bitwise ordinary-path parity");
  check(aliased_compact_addresses.size() == 3 &&
            aliased_compact_addresses[0] != aliased_compact_addresses[1] &&
            aliased_compact_addresses[1] == aliased_compact_addresses[2] &&
            aliased_ordinary_addresses.size() == 3 &&
            aliased_ordinary_addresses[0] != aliased_ordinary_addresses[1] &&
            aliased_ordinary_addresses[1] != aliased_ordinary_addresses[2],
        "compact mutation resumes after one copy-on-write update");
  close(aliased.value, 102, "outgoing aliases preserve snapshot values");
  close(aliased.gradient[0], 8,
        "outgoing aliases preserve snapshot gradients");

  ordinary_update_forward_calls = ordinary_update_backward_calls = 0;
  Executor loop_aliased_compact(outer(loop_aliased_update_plan(false)));
  Executor loop_aliased_ordinary(outer(loop_aliased_update_plan(true)));
  range_update_forward_addresses.clear();
  const Evaluation loop_aliased = evaluate(loop_aliased_compact, .25, 0);
  const auto loop_aliased_compact_addresses = range_update_forward_addresses;
  range_update_forward_addresses.clear();
  const Evaluation loop_aliased_reference =
      evaluate(loop_aliased_ordinary, .25, 0);
  const auto loop_aliased_ordinary_addresses = range_update_forward_addresses;
  check(std::memcmp(&loop_aliased, &loop_aliased_reference,
                    sizeof(Evaluation)) == 0 &&
            ordinary_update_forward_calls == 3 &&
            ordinary_update_backward_calls == 3,
        "loop-backedge alias invalidation has bitwise ordinary-path parity");
  check(loop_aliased_compact_addresses.size() == 3 &&
            loop_aliased_compact_addresses[0] !=
                loop_aliased_compact_addresses[1] &&
            loop_aliased_compact_addresses[1] ==
                loop_aliased_compact_addresses[2] &&
            loop_aliased_ordinary_addresses.size() == 3 &&
            loop_aliased_ordinary_addresses[0] !=
                loop_aliased_ordinary_addresses[1] &&
            loop_aliased_ordinary_addresses[1] !=
                loop_aliased_ordinary_addresses[2],
        "compact mutation resumes across an aliasing loop backedge");
  close(loop_aliased.value, 51.75,
        "loop-backedge alias preserves snapshot value");
  close(loop_aliased.gradient[0], 7,
        "loop-backedge alias preserves snapshot gradient");
}

static std::shared_ptr<StructuredLoop> integer_history_plan() {
  auto plan = std::make_shared<StructuredLoop>();
  const int theta = plan->body.add_slot(1, false);
  const int lower = scalar(*plan, -1);
  const int upper = scalar(*plan, 1);
  const int iterator = plan->body.add_slot(1, false);
  const int two = scalar(*plan, 2);
  const int integer_result = plan->body.add_slot(1, false);
  const int zero = scalar(*plan, 0);
  const int result = plan->body.add_slot(1, false);
  const int term = plan->body.add_slot(1, false);
  const int next = plan->body.add_slot(1, false);
  plan->imports = {{theta, 0, 0}};
  Node arithmetic = call(*plan, OP_INT_ARITH, {iterator, two}, integer_result);
  plan->body.ops[static_cast<size_t>(arithmetic.op)].variant = 2;
  Node multiply = call(*plan, OP_MUL, {theta, integer_result}, term);
  const int multiply_op = multiply.op;
  plan->root =
      sequence({alias(result, zero),
                counted(lower, upper, iterator, 3,
                        sequence({std::move(arithmetic), std::move(multiply),
                                  call(*plan, OP_ADD, {result, term}, next),
                                  alias(result, next)}))});
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  check(set_forward(plan->root, multiply_op, trace_iterator_forward),
        "find integer-result trace forward callback");
  check(set_backward(plan->root, multiply_op, trace_iterator_backward),
        "find integer-result trace backward callback");
  return plan;
}

static std::shared_ptr<StructuredLoop> comparison_target_plan() {
  auto plan = std::make_shared<StructuredLoop>();
  const int lower = scalar(*plan, -1);
  const int upper = scalar(*plan, 1);
  const int iterator = plan->body.add_slot(1, false);
  const int zero = scalar(*plan, 0);
  const int compared = plan->body.add_slot(1, false);
  Node comparison = call(*plan, OP_COMPARE, {iterator, zero}, compared);
  plan->body.ops[static_cast<size_t>(comparison.op)].variant = 0;
  Node target;
  target.kind = Node::Target;
  target.src = compared;
  plan->root = counted(lower, upper, iterator, 3,
                       sequence({std::move(comparison), std::move(target)}));
  plan->has_target = true;
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  return plan;
}

static std::shared_ptr<StructuredLoop> computed_nested_bound_plan() {
  auto plan = std::make_shared<StructuredLoop>();
  const int one = scalar(*plan, 1);
  const int two = scalar(*plan, 2);
  const int outer_iterator = plan->body.add_slot(1, false);
  const int inner_iterator = plan->body.add_slot(1, false);
  const int inner_upper = plan->body.add_slot(1, false);
  Node arithmetic =
      call(*plan, OP_INT_ARITH, {outer_iterator, one}, inner_upper);
  plan->body.ops[static_cast<size_t>(arithmetic.op)].variant = 0;
  Node target;
  target.kind = Node::Target;
  target.src = inner_iterator;
  plan->root = counted(
      one, two, outer_iterator, 2,
      sequence({std::move(arithmetic), counted(one, inner_upper, inner_iterator,
                                               3, std::move(target))}));
  plan->has_target = true;
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  return plan;
}

static std::shared_ptr<StructuredLoop> integer_output_plan(double a, double b) {
  auto plan = std::make_shared<StructuredLoop>();
  const int left = scalar(*plan, a);
  const int right = scalar(*plan, b);
  const int integer_result = plan->body.add_slot(1, false);
  const int result = plan->body.add_slot(1, false);
  Node arithmetic = call(*plan, OP_INT_ARITH, {left, right}, integer_result);
  plan->body.ops[static_cast<size_t>(arithmetic.op)].variant = 0;
  plan->root = sequence({std::move(arithmetic), alias(result, integer_result)});
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  return plan;
}

static std::shared_ptr<StructuredLoop> imported_integer_output_plan(
    int variant = 0) {
  auto plan = std::make_shared<StructuredLoop>();
  const int left = plan->body.add_slot(1, false);
  const int right = plan->body.add_slot(1, false);
  const int result = plan->body.add_slot(1, false);
  plan->imports = {{left, 0, 0}, {right, 1, 0}};
  Node arithmetic = call(*plan, OP_INT_ARITH, {left, right}, result);
  plan->body.ops[static_cast<size_t>(arithmetic.op)].variant = variant;
  plan->root = std::move(arithmetic);
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  return plan;
}

static int custom_integer_calls = 0;
static void custom_integer_forward(KernelCtx& context) {
  ++custom_integer_calls;
  find_kernel(OP_INT_ARITH)->forward(context);
  context.out.data[0] += 0.5;
}

static void registered_custom_integer_forward(KernelCtx& context) {
  ++custom_integer_calls;
  context.out.data[0] = 5.5;
}

struct ScopedKernelOverride {
  uint16_t opcode;
  Kernel saved;

  ScopedKernelOverride(uint16_t opcode, Kernel replacement)
      : opcode(opcode), saved(*find_kernel(opcode)) {
    register_kernel(opcode, replacement);
  }
  ~ScopedKernelOverride() { register_kernel(opcode, saved); }
};

static int inactive_workspace_calls = 0;
static double* inactive_workspace_address = nullptr;
static bool inactive_workspace_stable = true;
static bool inactive_workspace_disjoint = true;
static bool inactive_workspace_throw_once = false;

static int64_t inactive_workspace_size(const Op&, const Slot*) { return 3; }

static void inactive_workspace_forward(KernelCtx& context) {
  ++inactive_workspace_calls;
  if (!inactive_workspace_address) {
    inactive_workspace_address = context.scratch;
  } else {
    inactive_workspace_stable &= inactive_workspace_address == context.scratch;
  }
  inactive_workspace_disjoint &=
      context.scratch && context.scratch != context.in[0].data &&
      context.scratch != context.in[1].data &&
      context.scratch != context.out.data &&
      (!context.out2.data || context.scratch != context.out2.data) &&
      (context.out.len == 0 || !context.out2.data || context.out2.len == 0 ||
       context.out.data != context.out2.data);
  context.scratch[0] = context.in[0].data[0];
  context.scratch[1] = context.in[1].data[0];
  context.scratch[2] = context.scratch[0] + context.scratch[1];
  if (inactive_workspace_throw_once && context.scratch[0] == 2) {
    inactive_workspace_throw_once = false;
    throw std::runtime_error("injected inactive workspace failure");
  }
  if (context.out.len > 0) context.out.data[0] = context.scratch[2];
  if (context.out2.data)
    context.out2.data[0] = context.scratch[0] * context.scratch[1];
}

static std::shared_ptr<StructuredLoop> inactive_workspace_plan() {
  auto plan = std::make_shared<StructuredLoop>();
  const int lower = scalar(*plan, 1);
  const int upper = scalar(*plan, 3);
  const int iterator = plan->body.add_slot(1, false);
  const int two = scalar(*plan, 2);
  const int first = plan->body.add_slot(1, false);
  const int second = plan->body.add_slot(1, false);
  const int combined = plan->body.add_slot(1, false);
  const int zero = scalar(*plan, 0);
  const int result = plan->body.add_slot(1, false);
  Node workspace = call(*plan, OP_INT_ARITH, {iterator, two}, first);
  plan->body.ops[static_cast<size_t>(workspace.op)].out2 = second;
  plan->root = sequence(
      {alias(result, zero),
       counted(lower, upper, iterator, 3,
               sequence({std::move(workspace),
                         call(*plan, OP_ADD, {first, second}, combined),
                         alias(result, combined)}))});
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  return plan;
}

static std::shared_ptr<StructuredLoop> inactive_zero_output_plan() {
  auto plan = std::make_shared<StructuredLoop>();
  const int lower = scalar(*plan, 1);
  const int upper = scalar(*plan, 3);
  const int iterator = plan->body.add_slot(1, false);
  const int two = scalar(*plan, 2);
  const int empty = plan->body.add_slot(0, false);
  const int second_empty = plan->body.add_slot(0, false);
  const int second = plan->body.add_slot(1, false);
  const int zero = scalar(*plan, 0);
  const int result = plan->body.add_slot(1, false);
  Node empty_call = call(*plan, OP_INT_ARITH, {iterator, two}, empty);
  Node second_call = call(*plan, OP_INT_ARITH, {iterator, two}, second_empty);
  plan->body.ops[static_cast<size_t>(second_call.op)].out2 = second;
  plan->root =
      sequence({alias(result, zero),
                counted(lower, upper, iterator, 3,
                        sequence({std::move(empty_call), std::move(second_call),
                                  alias(result, second)}))});
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  return plan;
}

static void redirect_inactive_output(KernelCtx& context) {
  context.out.data = context.in[0].data;
}

static std::shared_ptr<StructuredLoop> redirected_inactive_output_plan() {
  auto plan = std::make_shared<StructuredLoop>();
  const int source = scalar(*plan, 42);
  const int zero = scalar(*plan, 0);
  const int result = plan->body.add_slot(1, false);
  Node redirected = call(*plan, OP_ADD, {source, zero}, result);
  const int redirected_op = redirected.op;
  plan->root = std::move(redirected);
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  check(set_forward(plan->root, redirected_op, redirect_inactive_output),
        "find redirected inactive callback");
  return plan;
}

static std::shared_ptr<StructuredLoop> inactive_location_target_plan() {
  auto plan = std::make_shared<StructuredLoop>();
  const int two = scalar(*plan, 2);
  const int three = scalar(*plan, 3);
  const int result = plan->body.add_slot(1, false);
  Node target;
  target.kind = Node::Target;
  target.src = result;
  plan->root =
      sequence({call(*plan, OP_ADD, {two, three}, result), std::move(target)});
  plan->has_target = true;
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  return plan;
}

static std::shared_ptr<StructuredLoop> inactive_location_bound_plan() {
  auto plan = std::make_shared<StructuredLoop>();
  const int one = scalar(*plan, 1);
  const int two = scalar(*plan, 2);
  const int upper = plan->body.add_slot(1, false);
  const int iterator = plan->body.add_slot(1, false);
  const int result = plan->body.add_slot(1, false);
  plan->root =
      sequence({call(*plan, OP_ADD, {one, two}, upper),
                counted(one, upper, iterator, 3, alias(result, iterator))});
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  return plan;
}

static std::shared_ptr<StructuredLoop> inactive_location_reverse_plan() {
  auto plan = std::make_shared<StructuredLoop>();
  const int theta = plan->body.add_slot(1, false);
  const int two = scalar(*plan, 2);
  const int three = scalar(*plan, 3);
  const int inactive = plan->body.add_slot(1, false);
  const int result = plan->body.add_slot(1, false);
  plan->imports = {{theta, 0, 0}};
  plan->root = sequence({call(*plan, OP_ADD, {two, three}, inactive),
                         call(*plan, OP_MUL, {theta, inactive}, result)});
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  return plan;
}

static std::shared_ptr<StructuredLoop> inactive_location_update_plan() {
  auto plan = std::make_shared<StructuredLoop>();
  const int lower = scalar(*plan, 1);
  const int upper = scalar(*plan, 3);
  const int iterator = plan->body.add_slot(1, false);
  const int base = plan->body.add_slot(3, false);
  plan->fills.push_back({base, {10, 20, 30}});
  const int current = plan->body.add_slot(3, false);
  const int rhs = scalar(*plan, 7);
  const int updated = plan->body.add_slot(3, false);
  const int result = plan->body.add_slot(1, false);
  auto spec = std::make_shared<DynamicIndexSpec>();
  spec->axes = {{DynamicIndexSpec::Axis::Single, 3, 1, 1, 0}};
  spec->selected_size = 1;
  Node update =
      call(*plan, OP_SET_INDEX_DYNAMIC, {current, iterator, rhs}, updated);
  plan->body.ops[static_cast<size_t>(update.op)].udata = spec.get();
  plan->body.udata_pool.push_back(spec);
  plan->root =
      sequence({alias(current, base),
                counted(lower, upper, iterator, 3,
                        sequence({std::move(update), alias(current, updated)})),
                call(*plan, OP_SUM_VEC, {current}, result)});
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  check(plan->compact_update_sites == 1,
        "inactive location update selects compact history");
  return plan;
}

static int active_workspace_forward_calls = 0;
static int active_workspace_backward_calls = 0;
static double* active_workspace_address = nullptr;
static bool active_workspace_history_ok = true;

static void active_workspace_forward(KernelCtx& context) {
  ++active_workspace_forward_calls;
  active_workspace_address = context.scratch;
  context.scratch[0] = context.in[0].data[0];
  context.scratch[1] = context.in[1].data[0];
  context.scratch[2] = context.scratch[0] * context.scratch[1];
  context.out.data[0] = context.scratch[2];
  context.out2.data[0] = context.scratch[0] + context.scratch[1];
}

static void active_workspace_backward(KernelCtx& context) {
  ++active_workspace_backward_calls;
  active_workspace_history_ok &=
      context.scratch == active_workspace_address &&
      context.scratch[0] == context.in[0].data[0] &&
      context.scratch[1] == context.in[1].data[0] &&
      context.scratch[2] == context.in[0].data[0] * context.in[1].data[0];
  if (context.in_adj[0].data)
    context.in_adj[0].data[0] +=
        context.scratch[1] * context.out_adj + context.out2_adj;
  if (context.in_adj[1].data)
    context.in_adj[1].data[0] +=
        context.scratch[0] * context.out_adj + context.out2_adj;
}

static std::shared_ptr<StructuredLoop> active_workspace_plan() {
  auto plan = std::make_shared<StructuredLoop>();
  const int theta = plan->body.add_slot(1, false);
  const int two = scalar(*plan, 2);
  const int product = plan->body.add_slot(1, false);
  const int sum = plan->body.add_slot(1, false);
  const int result = plan->body.add_slot(1, false);
  plan->imports = {{theta, 0, 0}};
  Node workspace = call(*plan, OP_INT_ARITH, {theta, two}, product);
  plan->body.ops[static_cast<size_t>(workspace.op)].out2 = sum;
  plan->root = sequence(
      {std::move(workspace), call(*plan, OP_ADD, {product, sum}, result)});
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  return plan;
}

static std::shared_ptr<StructuredLoop> custom_integer_plan() {
  auto plan = integer_output_plan(2, 3);
  check(set_forward(plan->root, 0, custom_integer_forward),
        "find custom integer callback");
  return plan;
}

static void compact_integer_result_tests() {
  iterator_forward_values.clear();
  iterator_reverse_values.clear();
  Executor history(outer(integer_history_plan()));
  const Evaluation traced = evaluate(history, .25, 0);
  close(traced.value, 0, "inline integer history value");
  close(traced.gradient[0], 0, "inline integer history theta gradient");
  check(iterator_forward_values == std::vector<double>({-2, 0, 2}),
        "inline integer forward values are exact");
  check(iterator_reverse_values == std::vector<double>({2, 0, -2}),
        "inline integer reverse values preserve history");

  Executor comparison_target(outer(comparison_target_plan()));
  const Evaluation compared = evaluate(comparison_target, 0, 0);
  close(compared.value, 1, "inline comparison target keeps reached values");
  close(compared.gradient[0], 0, "inline comparison target theta gradient");
  close(compared.gradient[1], 0, "inline comparison target beta gradient");

  Executor nested_bound(outer(computed_nested_bound_plan()));
  const Evaluation nested = evaluate(nested_bound, 0, 0);
  close(nested.value, 9, "inline integer supplies nested loop bound");
  close(nested.gradient[0], 0, "inline integer bound theta gradient");
  close(nested.gradient[1], 0, "inline integer bound beta gradient");

  for (const auto& point : std::vector<std::pair<double, double>>{
           {static_cast<double>(std::numeric_limits<int32_t>::min()), 0},
           {static_cast<double>(std::numeric_limits<int32_t>::max()) - 1, 1}}) {
    Executor boundary(outer(integer_output_plan(point.first, point.second)));
    const Evaluation result = evaluate(boundary, 0, 0);
    close(result.value, point.first + point.second,
          "inline integer boundary escapes through output alias");
    close(result.gradient[0], 0, "inline integer boundary theta gradient");
    close(result.gradient[1], 0, "inline integer boundary beta gradient");
  }

  Executor retry(outer(imported_integer_output_plan()));
  bool threw = false;
  try {
    (void)evaluate(retry,
                   static_cast<double>(std::numeric_limits<int32_t>::max()), 1);
  } catch (const std::domain_error& error) {
    threw = std::string(error.what()) ==
            "integer arithmetic exceeds Stan integer range";
  }
  check(threw, "inline integer preserves arithmetic exception");
  const Evaluation retried = evaluate(retry, 2, 3);
  close(retried.value, 5, "inline integer retry value");
  close(retried.gradient[0], 0, "inline integer retry theta gradient");
  close(retried.gradient[1], 0, "inline integer retry beta gradient");

  Executor division_retry(outer(imported_integer_output_plan(3)));
  threw = false;
  try {
    (void)evaluate(division_retry, 4, 0);
  } catch (const std::domain_error& error) {
    threw = std::string(error.what()) == "integer division by zero";
  }
  check(threw, "inline integer preserves division exception");
  const Evaluation divided = evaluate(division_retry, 4, 2);
  close(divided.value, 2, "inline integer division retry value");
  close(divided.gradient[0], 0,
        "inline integer division retry theta gradient");
  close(divided.gradient[1], 0,
        "inline integer division retry beta gradient");

  custom_integer_calls = 0;
  Executor custom(outer(custom_integer_plan()));
  const Evaluation custom_result = evaluate(custom, 0, 0);
  close(custom_result.value, 5.5,
        "custom integer callback keeps ordinary result semantics");
  check(custom_integer_calls == 1,
        "custom integer callback uses ordinary retained path");

  custom_integer_calls = 0;
  {
    Kernel replacement = *find_kernel(OP_INT_ARITH);
    replacement.forward = registered_custom_integer_forward;
    ScopedKernelOverride overridden(OP_INT_ARITH, replacement);
    Executor registered_custom(outer(integer_output_plan(2, 3)));
    const Evaluation registered_result = evaluate(registered_custom, 0, 0);
    close(registered_result.value, 5.5,
          "registered custom integer callback is not inlined");
  }
  check(custom_integer_calls == 1,
        "pre-prepare custom integer callback keeps ordinary path");

  inactive_workspace_calls = 0;
  inactive_workspace_address = nullptr;
  inactive_workspace_stable = true;
  inactive_workspace_disjoint = true;
  inactive_workspace_throw_once = true;
  {
    Kernel replacement = *find_kernel(OP_INT_ARITH);
    replacement.forward = inactive_workspace_forward;
    replacement.scratch_size = inactive_workspace_size;
    ScopedKernelOverride overridden(OP_INT_ARITH, replacement);
    Executor workspace(outer(inactive_workspace_plan()));
    bool workspace_threw = false;
    try {
      (void)evaluate(workspace, 0, 0);
    } catch (const std::runtime_error& error) {
      workspace_threw =
          std::string(error.what()) == "injected inactive workspace failure";
    }
    check(workspace_threw, "inactive workspace preserves callback exception");
    const Evaluation workspace_result = evaluate(workspace, 0, 0);
    close(workspace_result.value, 11,
          "inactive workspace preserves second output and retry");
    close(workspace_result.gradient[0], 0,
          "inactive workspace retry theta gradient");
    close(workspace_result.gradient[1], 0,
          "inactive workspace retry beta gradient");
  }
  check(inactive_workspace_calls == 5,
        "inactive workspace executes reached callbacks only");
  check(inactive_workspace_stable,
        "inactive callback scratch is reused across calls and retry");
  check(inactive_workspace_disjoint,
        "inactive callback workspace descriptors remain disjoint");

  inactive_workspace_calls = 0;
  inactive_workspace_address = nullptr;
  inactive_workspace_stable = true;
  inactive_workspace_disjoint = true;
  inactive_workspace_throw_once = false;
  {
    Kernel replacement = *find_kernel(OP_INT_ARITH);
    replacement.forward = inactive_workspace_forward;
    replacement.scratch_size = inactive_workspace_size;
    ScopedKernelOverride overridden(OP_INT_ARITH, replacement);
    Executor zero_output(outer(inactive_zero_output_plan()));
    const Evaluation result = evaluate(zero_output, 0, 0);
    close(result.value, 6,
          "inactive workspace preserves zero primary and second output");
  }
  check(inactive_workspace_calls == 6,
        "inactive zero-output callbacks execute without arena anchors");
  check(inactive_workspace_stable && inactive_workspace_disjoint,
        "inactive zero-output workspace remains stable and disjoint");

  Executor redirected(outer(redirected_inactive_output_plan()));
  const Evaluation redirected_result = evaluate(redirected, 0, 0);
  close(redirected_result.value, 42,
        "redirected inactive output keeps ordinary reference fallback");

  Executor inactive_target(outer(inactive_location_target_plan()));
  const Evaluation targeted = evaluate(inactive_target, 0, 0);
  close(targeted.value, 5, "inactive arena handle reaches target reduction");

  Executor inactive_bound(outer(inactive_location_bound_plan()));
  const Evaluation bounded = evaluate(inactive_bound, 0, 0);
  close(bounded.value, 3, "inactive arena handle supplies counted-loop bound");

  Executor inactive_reverse(outer(inactive_location_reverse_plan()));
  const Evaluation reversed = evaluate(inactive_reverse, 4, 0);
  close(reversed.value, 20,
        "active operation reads inactive arena-handle primal");
  close(reversed.gradient[0], 5,
        "reverse operation reads inactive arena-handle primal");
  close(reversed.gradient[1], 0,
        "inactive arena-handle input has no adjoint storage");

  Executor inactive_update(outer(inactive_location_update_plan()));
  const Evaluation updated = evaluate(inactive_update, 0, 0);
  close(updated.value, 21,
        "inactive arena handle supplies compact-update base");

  active_workspace_forward_calls = 0;
  active_workspace_backward_calls = 0;
  active_workspace_address = nullptr;
  active_workspace_history_ok = true;
  {
    Kernel replacement = *find_kernel(OP_INT_ARITH);
    replacement.forward = active_workspace_forward;
    replacement.backward = active_workspace_backward;
    replacement.scratch_size = inactive_workspace_size;
    ScopedKernelOverride overridden(OP_INT_ARITH, replacement);
    Executor active_workspace(outer(active_workspace_plan()));
    const Evaluation active = evaluate(active_workspace, 3, 0);
    close(active.value, 11, "active scratchful callback value");
    close(active.gradient[0], 3, "active scratchful callback gradient");
    close(active.gradient[1], 0, "active scratchful callback beta gradient");
  }
  check(active_workspace_forward_calls == 1 &&
            active_workspace_backward_calls == 1,
        "active scratchful callback retains reverse record");
  check(active_workspace_history_ok,
        "active scratchful callback retains forward scratch for reverse");
}

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
  plan->root =
      sequence({alias(result, theta),
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
  check(invariant_variant_calls == 3, "iterator-dependent work is not reused");
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
  Node comparison =
      call(*late, OP_COMPARE, {late_iterator, late_one}, late_condition);
  late->body.ops[static_cast<size_t>(comparison.op)].variant = 2;
  Node late_definition = call(*late, OP_ADD, {late_one, late_one}, late_source);
  const int late_definition_op = late_definition.op;
  Node late_use = call(*late, OP_ADD, {late_source, late_one}, late_result);
  const int late_use_op = late_use.op;
  late->root = counted(
      late_lower, late_upper, late_iterator, 3,
      sequence(
          {std::move(comparison),
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
      sequence(
          {std::move(while_constant),
           call(*while_plan, OP_ADD, {while_counter, while_one}, while_next),
           alias(while_counter, while_next),
           alias(while_result, while_invariant)}));
  while_plan->outputs = {while_result};
  while_plan->prepare(1 << 20);
  while_plan->dynamic_history = true;
  check(set_forward(while_plan->root, while_constant_op, count_invariant_first),
        "find while invariant callback");
  invariant_first_calls = 0;
  Executor while_executor(outer(while_plan));
  const Evaluation while_result_value = evaluate_invariant(while_executor, 0);
  close(while_result_value.value, 2, "while invariant result");
  check(invariant_first_calls == 1,
        "while body reuses its inactive invariant result");
}

static std::vector<double*> control_first_outputs;
static std::vector<double*> control_second_outputs;
static std::atomic<int> control_backward_calls{0};
static std::atomic<bool> control_throw_once{false};

static void record_control_add(KernelCtx& context) {
  control_first_outputs.push_back(context.out.data);
  find_kernel(OP_ADD)->forward(context);
}
static void record_control_compare(KernelCtx& context) {
  control_second_outputs.push_back(context.out.data);
  find_kernel(OP_COMPARE)->forward(context);
}
static void record_control_add_backward(KernelCtx& context) {
  ++control_backward_calls;
  find_kernel(OP_ADD)->backward(context);
}
static void throw_then_record_control_add(KernelCtx& context) {
  if (control_throw_once.exchange(false))
    throw std::runtime_error("injected inactive-control failure");
  record_control_add(context);
}

static bool one_address(const std::vector<double*>& addresses) {
  return !addresses.empty() &&
         std::all_of(addresses.begin(), addresses.end(), [&](double* address) {
           return address == addresses.front();
         });
}

static bool distinct_addresses(const std::vector<double*>& addresses) {
  for (size_t i = 0; i < addresses.size(); ++i)
    for (size_t j = 0; j < i; ++j)
      if (addresses[i] == addresses[j]) return false;
  return true;
}

static std::shared_ptr<StructuredLoop> control_cone_plan(bool active_control) {
  auto plan = std::make_shared<StructuredLoop>();
  const int theta = plan->body.add_slot(1, false);
  const int lower = scalar(*plan, 1);
  const int upper = scalar(*plan, 3);
  const int iterator = plan->body.add_slot(1, false);
  const int zero = scalar(*plan, 0);
  const int limit = scalar(*plan, 3);
  const int first = plan->body.add_slot(1, false);
  const int active_middle = plan->body.add_slot(1, false);
  const int condition = plan->body.add_slot(1, false);
  const int result = plan->body.add_slot(1, false);
  const int updated = plan->body.add_slot(1, false);
  plan->imports = {{theta, 0, 0}};
  Node first_call = call(*plan, OP_ADD, {iterator, zero}, first);
  const int first_op = first_call.op;
  std::vector<Node> control;
  control.push_back(std::move(first_call));
  int compare_input = first;
  int active_middle_op = -1;
  if (active_control) {
    Node middle = call(*plan, OP_ADD, {first, theta}, active_middle);
    active_middle_op = middle.op;
    compare_input = active_middle;
    control.push_back(std::move(middle));
  }
  Node compare = call(*plan, OP_COMPARE, {compare_input, limit}, condition);
  plan->body.ops[static_cast<size_t>(compare.op)].variant = 0;
  const int compare_op = compare.op;
  control.push_back(std::move(compare));
  Node update = call(*plan, OP_ADD, {result, theta}, updated);
  control.push_back(
      branch(condition, sequence({std::move(update), alias(result, updated)}),
             sequence({})));
  plan->root =
      sequence({alias(result, zero), counted(lower, upper, iterator, 3,
                                             sequence(std::move(control)))});
  plan->outputs = {result};
  plan->prepare(1 << 20);
  plan->dynamic_history = true;
  check(set_forward(plan->root, first_op, record_control_add),
        "find inactive-control first callback");
  check(set_forward(plan->root, compare_op, record_control_compare),
        "find inactive-control terminal callback");
  if (active_control)
    check(
        set_backward(plan->root, active_middle_op, record_control_add_backward),
        "find later active control-cone backward callback");
  return plan;
}

static void inactive_control_tests() {
  test_unsetenv("STANLI_NO_STRUCTURED_INACTIVE_CONTROL");
  auto plan = control_cone_plan(false);
  Executor enabled(outer(plan));
  control_first_outputs.clear();
  control_second_outputs.clear();
  const Evaluation direct = evaluate_invariant(enabled, .25);
  close(direct.value, .5, "inactive-control branch result");
  close(direct.gradient[0], 2, "inactive-control branch gradient");
  check(control_first_outputs.size() == 3 &&
            control_second_outputs.size() == 3 &&
            one_address(control_first_outputs) &&
            one_address(control_second_outputs),
        "inactive control cone uses body-sized canonical outputs");

  test_setenv("STANLI_NO_STRUCTURED_INACTIVE_CONTROL", "1");
  Executor disabled(outer(plan));
  control_first_outputs.clear();
  control_second_outputs.clear();
  const Evaluation ordinary = evaluate_invariant(disabled, .25);
  test_unsetenv("STANLI_NO_STRUCTURED_INACTIVE_CONTROL");
  check(std::memcmp(&direct, &ordinary, sizeof(Evaluation)) == 0,
        "inactive-control elision has bitwise same-binary parity");
  check(control_first_outputs.size() == 3 &&
            distinct_addresses(control_first_outputs),
        "inactive-control ablation retains per-call outputs");

  // An active external handle used only by a later site rejects the complete
  // cone before its inactive first output is overwritten. Reverse must retain
  // and invoke the active middle callback.
  auto active_plan = control_cone_plan(true);
  Executor active_executor(outer(active_plan));
  control_first_outputs.clear();
  control_second_outputs.clear();
  control_backward_calls = 0;
  const Evaluation active = evaluate_invariant(active_executor, .25);
  close(active.value, .5, "later-active control-cone result");
  close(active.gradient[0], 2, "later-active control-cone gradient");
  check(control_first_outputs.size() == 3 &&
            distinct_addresses(control_first_outputs),
        "later active input makes the whole control cone ordinary");
  check(control_backward_calls == 3,
        "later active control site preserves zero-seed reverse callbacks");

  // H6C can reuse an invariant first member while H6B continues direct
  // execution for the iterator-dependent remainder of the same cone. The
  // early H6C return must still finish its H6B site and avoid cone reentry.
  auto interaction_plan = std::make_shared<StructuredLoop>();
  const int interaction_theta = interaction_plan->body.add_slot(1, false);
  const int interaction_lower = scalar(*interaction_plan, 1);
  const int interaction_upper = scalar(*interaction_plan, 3);
  const int interaction_iterator = interaction_plan->body.add_slot(1, false);
  const int interaction_zero = scalar(*interaction_plan, 0);
  const int interaction_two = scalar(*interaction_plan, 2);
  const int interaction_three = scalar(*interaction_plan, 3);
  const int interaction_limit = scalar(*interaction_plan, 8);
  const int interaction_invariant = interaction_plan->body.add_slot(1, false);
  const int interaction_variant = interaction_plan->body.add_slot(1, false);
  const int interaction_condition = interaction_plan->body.add_slot(1, false);
  const int interaction_result = interaction_plan->body.add_slot(1, false);
  const int interaction_updated = interaction_plan->body.add_slot(1, false);
  interaction_plan->imports = {{interaction_theta, 0, 0}};
  Node interaction_first =
      call(*interaction_plan, OP_ADD, {interaction_two, interaction_three},
           interaction_invariant);
  const int interaction_first_op = interaction_first.op;
  Node interaction_later =
      call(*interaction_plan, OP_ADD,
           {interaction_iterator, interaction_invariant}, interaction_variant);
  const int interaction_later_op = interaction_later.op;
  Node interaction_compare =
      call(*interaction_plan, OP_COMPARE,
           {interaction_variant, interaction_limit}, interaction_condition);
  interaction_plan->body.ops[static_cast<size_t>(interaction_compare.op)]
      .variant = 0;
  const int interaction_compare_op = interaction_compare.op;
  Node interaction_update =
      call(*interaction_plan, OP_ADD, {interaction_result, interaction_theta},
           interaction_updated);
  interaction_plan->root = sequence(
      {alias(interaction_result, interaction_zero),
       counted(
           interaction_lower, interaction_upper, interaction_iterator, 3,
           sequence({std::move(interaction_first), std::move(interaction_later),
                     std::move(interaction_compare),
                     branch(interaction_condition,
                            sequence({std::move(interaction_update),
                                      alias(interaction_result,
                                            interaction_updated)}),
                            sequence({}))}))});
  interaction_plan->outputs = {interaction_result};
  interaction_plan->prepare(1 << 20);
  interaction_plan->dynamic_history = true;
  check(set_forward(interaction_plan->root, interaction_first_op,
                    count_invariant_first),
        "find H6C member of inactive control cone");
  check(set_forward(interaction_plan->root, interaction_later_op,
                    record_control_add),
        "find later H6B member after H6C reuse");
  check(set_forward(interaction_plan->root, interaction_compare_op,
                    record_control_compare),
        "find terminal H6B member after H6C reuse");
  invariant_first_calls = 0;
  control_first_outputs.clear();
  control_second_outputs.clear();
  Executor interaction_executor(outer(interaction_plan));
  const Evaluation interaction = evaluate_invariant(interaction_executor, .25);
  close(interaction.value, .5, "H6C and H6B interaction result");
  close(interaction.gradient[0], 2, "H6C and H6B interaction gradient");
  check(invariant_first_calls == 1,
        "H6C reuses the first inactive control-cone member");
  check(control_first_outputs.size() == 3 &&
            control_second_outputs.size() == 3 &&
            one_address(control_first_outputs) &&
            one_address(control_second_outputs),
        "H6B continues direct execution after H6C reuse");

  // While guards include the final false condition evaluation.
  auto while_plan = std::make_shared<StructuredLoop>();
  const int while_theta = while_plan->body.add_slot(1, false);
  const int counter = scalar(*while_plan, 0);
  const int zero = scalar(*while_plan, 0);
  const int one = scalar(*while_plan, 1);
  const int three = scalar(*while_plan, 3);
  const int first = while_plan->body.add_slot(1, false);
  const int condition = while_plan->body.add_slot(1, false);
  const int next = while_plan->body.add_slot(1, false);
  while_plan->imports = {{while_theta, 0, 0}};
  Node first_call = call(*while_plan, OP_ADD, {counter, zero}, first);
  const int first_op = first_call.op;
  Node compare = call(*while_plan, OP_COMPARE, {three, first}, condition);
  while_plan->body.ops[static_cast<size_t>(compare.op)].variant = 2;
  const int compare_op = compare.op;
  while_plan->root = while_loop(
      condition, 3, sequence({std::move(first_call), std::move(compare)}),
      sequence({call(*while_plan, OP_ADD, {counter, one}, next),
                alias(counter, next)}));
  while_plan->outputs = {counter};
  while_plan->prepare(1 << 20);
  while_plan->dynamic_history = true;
  check(set_forward(while_plan->root, first_op, record_control_add),
        "find while control-cone first callback");
  check(set_forward(while_plan->root, compare_op, record_control_compare),
        "find while control-cone terminal callback");
  control_first_outputs.clear();
  control_second_outputs.clear();
  Executor while_executor(outer(while_plan));
  const Evaluation while_value = evaluate_invariant(while_executor, 0);
  close(while_value.value, 3, "inactive-control while result");
  check(control_first_outputs.size() == 4 &&
            control_second_outputs.size() == 4 &&
            one_address(control_first_outputs),
        "inactive-control while includes its final false guard");

  // A value that escapes the cone cannot use canonical storage because a
  // later iteration could overwrite a published historical value.
  auto escaped_plan = std::make_shared<StructuredLoop>();
  const int escaped_theta = escaped_plan->body.add_slot(1, false);
  const int escaped_lower = scalar(*escaped_plan, 1);
  const int escaped_upper = scalar(*escaped_plan, 3);
  const int escaped_iterator = escaped_plan->body.add_slot(1, false);
  const int escaped_zero = scalar(*escaped_plan, 0);
  const int escaped_limit = scalar(*escaped_plan, 3);
  const int escaped_first = escaped_plan->body.add_slot(1, false);
  const int escaped_condition = escaped_plan->body.add_slot(1, false);
  const int escaped_output = escaped_plan->body.add_slot(1, false);
  escaped_plan->imports = {{escaped_theta, 0, 0}};
  Node escaped_add = call(*escaped_plan, OP_ADD,
                          {escaped_iterator, escaped_zero}, escaped_first);
  const int escaped_add_op = escaped_add.op;
  Node escaped_compare =
      call(*escaped_plan, OP_COMPARE, {escaped_first, escaped_limit},
           escaped_condition);
  escaped_plan->body.ops[static_cast<size_t>(escaped_compare.op)].variant = 0;
  escaped_plan->root =
      counted(escaped_lower, escaped_upper, escaped_iterator, 3,
              sequence({std::move(escaped_add), std::move(escaped_compare),
                        branch(escaped_condition, sequence({}), sequence({})),
                        alias(escaped_output, escaped_first)}));
  escaped_plan->outputs = {escaped_output};
  escaped_plan->prepare(1 << 20);
  escaped_plan->dynamic_history = true;
  check(set_forward(escaped_plan->root, escaped_add_op, record_control_add),
        "find escaping control value callback");
  control_first_outputs.clear();
  Executor escaped_executor(outer(escaped_plan));
  const Evaluation escaped = evaluate_invariant(escaped_executor, 0);
  close(escaped.value, 3, "escaping control value result");
  check(control_first_outputs.size() == 3 &&
            distinct_addresses(control_first_outputs),
        "escaping control value retains ordinary history");

  // If a cone callback throws after direct execution starts, a fresh forward
  // must rebuild its handles and start the cone from a clean state.
  auto retry_plan = control_cone_plan(false);
  int retry_op = -1;
  std::function<void(const Node&)> find_first = [&](const Node& node) {
    if (retry_op >= 0) return;
    if (node.kind == Node::KernelCall &&
        retry_plan->body.ops[static_cast<size_t>(node.op)].opcode == OP_ADD) {
      const Op& op = retry_plan->body.ops[static_cast<size_t>(node.op)];
      if (op.in[0] == 3) retry_op = node.op;
    }
    for (const auto& child : node.children) find_first(child);
  };
  find_first(retry_plan->root);
  check(retry_op >= 0 && set_forward(retry_plan->root, retry_op,
                                     throw_then_record_control_add),
        "find retry control-cone callback");
  Executor retry_executor(outer(retry_plan));
  control_throw_once = true;
  bool threw = false;
  try {
    (void)evaluate_invariant(retry_executor, .25);
  } catch (const std::runtime_error& error) {
    threw = std::string(error.what()) == "injected inactive-control failure";
  }
  check(threw, "inactive-control callback preserves forward exception");
  const Evaluation retry = evaluate_invariant(retry_executor, .25);
  check(std::memcmp(&direct, &retry, sizeof(Evaluation)) == 0,
        "inactive-control retry rebuilds clean execution state");
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

static void direct_index_lowering_tests() {
  test_unsetenv("STANLI_NO_STRUCTURED_DIRECT_INDEX_INPUTS");
  const auto direct =
      compile_fixture("structured_direct_index", 4, Mode::Force);
  test_setenv("STANLI_NO_STRUCTURED_DIRECT_INDEX_INPUTS", "1");
  const auto packed =
      compile_fixture("structured_direct_index", 4, Mode::Force);
  test_unsetenv("STANLI_NO_STRUCTURED_DIRECT_INDEX_INPUTS");
  const StructuredLoop* direct_plan = retained(direct);
  const StructuredLoop* packed_plan = retained(packed);
  check(direct_plan && packed_plan,
        "direct-index ablation keeps structured execution");
  size_t direct_reads = 0, direct_concats = 0, packed_reads = 0,
         packed_concats = 0;
  if (direct_plan)
    for (const auto& op : direct_plan->body.ops) {
      direct_concats += op.opcode == OP_CONCAT2;
      if (op.opcode == OP_SET_INDEX_DYNAMIC) {
        const auto* spec = static_cast<const DynamicIndexSpec*>(op.udata);
        check(spec && spec->input_count == 0 && op.n_in == 3,
              "indexed updates preserve the packed ABI");
      } else if (op.opcode == OP_INDEX_DYNAMIC) {
        const auto* spec = static_cast<const DynamicIndexSpec*>(op.udata);
        if (spec && spec->input_count > 0) {
          ++direct_reads;
          check(op.n_in == spec->input_count && op.n_in == 3,
                "two-axis read binds direct scalar selector inputs");
        }
      }
    }
  if (packed_plan)
    for (const auto& op : packed_plan->body.ops) {
      packed_concats += op.opcode == OP_CONCAT2;
      if (op.opcode != OP_INDEX_DYNAMIC) continue;
      ++packed_reads;
      const auto* spec = static_cast<const DynamicIndexSpec*>(op.udata);
      check(spec && spec->input_count == 0 && op.n_in == 2,
            "direct-index ablation preserves packed read ABI");
    }
  check(direct_reads > 0 && packed_reads >= direct_reads,
        "structured model exposes direct-index read sites");
  check(direct_concats < packed_concats,
        "direct scalar selectors remove packing operations");
  compare_gradients(direct, packed, {{.1, .7}, {-.2, .3}, {0, .5}},
                    "direct-index value parity",
                    "direct-index gradient parity");
  const auto legacy = compile_fixture("structured_direct_index", 4, Mode::Off);
  compare_gradients(direct, legacy, {{.1, .7}, {-.2, .3}},
                    "direct-index legacy value parity",
                    "direct-index legacy gradient parity");

  test_setenv("STANLI_STRUCTURED_HISTORY_BYTES", "1");
  test_unsetenv("STANLI_NO_STRUCTURED_DIRECT_INDEX_INPUTS");
  const auto dynamic_direct =
      compile_fixture("structured_direct_index", 4, Mode::Force);
  test_setenv("STANLI_NO_STRUCTURED_DIRECT_INDEX_INPUTS", "1");
  const auto dynamic_packed =
      compile_fixture("structured_direct_index", 4, Mode::Force);
  test_unsetenv("STANLI_NO_STRUCTURED_DIRECT_INDEX_INPUTS");
  test_unsetenv("STANLI_STRUCTURED_HISTORY_BYTES");
  check(retained(dynamic_direct) && retained(dynamic_direct)->dynamic_history,
        "direct-index test exercises dynamic history");
  compare_gradients(dynamic_direct, dynamic_packed, {{.1, .7}, {-.2, .3}},
                    "dynamic direct-index value parity",
                    "dynamic direct-index gradient parity");
}

int main() {
  test_unsetenv("STANLI_STRUCTURED_LOOP_DIAGNOSTICS");
  runtime_trip_tests();
  compact_import_reference_tests();
  direct_index_kernel_tests();
  forced_control_tests();
  dynamic_history_tests();
  compact_iterator_history_tests();
  compact_integer_result_tests();
  loop_invariant_reuse_tests();
  inactive_control_tests();
  dynamic_history_concurrency_tests();
  dynamic_history_failure_tests();
  automatic_policy_tests();
  direct_index_lowering_tests();
  test_unsetenv("STANLI_STRUCTURED_LOOPS");
  test_unsetenv("STANLI_STRUCTURED_HISTORY_BYTES");
  test_unsetenv("STANLI_NO_STRUCTURED_INVARIANT_REUSE");
  test_unsetenv("STANLI_NO_STRUCTURED_INACTIVE_CONTROL");
  test_unsetenv("STANLI_NO_STRUCTURED_DIRECT_INDEX_INPUTS");
  if (failures == 0) std::printf("test_structured_loop_production OK\n");
  return failures != 0;
}
