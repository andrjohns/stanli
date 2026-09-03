#include <stanli/structured_loop.hpp>
#include <stanli/optable.hpp>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <new>
#include <numeric>
#include <stdexcept>
#include <type_traits>

namespace stanli {
namespace {

// Offsets/control records are encoded exactly as doubles. This also avoids
// type-punning or alignment assumptions about the executor's double arena.
constexpr int64_t exact_limit = int64_t{1} << 52;
int64_t add(int64_t a, int64_t b) {
  if (a < 0 || b < 0 || a > exact_limit - b)
    throw std::length_error("structured loop storage overflow");
  return a + b;
}
int64_t mul(int64_t a, int64_t b) {
  if (a < 0 || b < 0 || (b != 0 && a > exact_limit / b))
    throw std::length_error("structured loop storage overflow");
  return a * b;
}
using Node = StructuredLoop::Node;
void index_forward(KernelCtx& c);
void index_backward(KernelCtx& c);
int64_t scalar_index_forward(KernelCtx& c);
void set_index_forward(KernelCtx& c);
void set_index_backward(KernelCtx& c);
struct IndexInputLayout {
  int expected = 0;
  int selector_end = 0;
  int rhs = -1;
};
bool index_input_layout(const DynamicIndexSpec& p, bool update,
                        IndexInputLayout& result) noexcept {
  if (p.input_count < 0 || p.input_count > 6) return false;
  if (!update) {
    if (p.rhs_input != -1) return false;
    if (p.input_count == 0) {
      result = {2, 2, -1};
      return true;
    }
    if (p.input_count < 2) return false;
    result = {p.input_count, p.input_count, -1};
    return true;
  }
  if (p.input_count == 0) {
    if (p.rhs_input != -1) return false;
    result = {3, 2, 2};
    return true;
  }
  if (p.input_count < 3 || p.rhs_input != p.input_count - 1) return false;
  result = {p.input_count, p.rhs_input, p.rhs_input};
  return true;
}
IndexInputLayout require_index_input_layout(const DynamicIndexSpec& p,
                                            bool update) {
  IndexInputLayout result;
  if (!index_input_layout(p, update, result))
    throw std::logic_error("invalid structured index input count");
  return result;
}
bool index_selection_is_ordered_unique(const DynamicIndexSpec& p) {
  return std::all_of(p.axes.begin(), p.axes.end(), [](const auto& axis) {
    return axis.kind != DynamicIndexSpec::Axis::Multi || axis.count <= 1;
  });
}
bool scalar_compact_update_spec(const DynamicIndexSpec& p) {
  return p.selected_size == 1 &&
         std::all_of(p.axes.begin(), p.axes.end(), [](const auto& axis) {
           return axis.kind == DynamicIndexSpec::Axis::Single &&
                  axis.count == 1 && axis.count_input_offset < 0;
         });
}
void slot(const StructuredLoop& p, int s) {
  if (s < 0 || static_cast<size_t>(s) >= p.body.slots.size())
    throw std::invalid_argument("structured loop invalid slot");
}
void scalar(const StructuredLoop& p, int s) {
  slot(p, s);
  if (p.body.slots[s].len != 1)
    throw std::invalid_argument("structured loop control needs a scalar");
}
void prepare_node(StructuredLoop& p, Node& n, unsigned depth,
                  unsigned loop_depth) {
  if (depth > 256) throw std::length_error("structured loop nesting limit");
  ++p.node_count;
  n.record_site = ~uint32_t{0};
  n.compact_update_cell = -1;
  n.frame_size = n.target_capacity = 0;
  const bool loop = n.kind == Node::For || n.kind == Node::While;
  for (auto& c : n.children) prepare_node(p, c, depth + 1, loop_depth + loop);
  switch (n.kind) {
    case Node::Sequence:
      n.frame_size = 1;  // executed child prefix, including a partial child
      for (const auto& c : n.children) {
        n.frame_size = add(n.frame_size, c.frame_size);
        n.target_capacity = add(n.target_capacity, c.target_capacity);
      }
      break;
    case Node::KernelCall: {
      if (p.record_node_count >= std::numeric_limits<uint32_t>::max())
        throw std::length_error("too many structured kernel call sites");
      n.record_site = static_cast<uint32_t>(p.record_node_count++);
      if (n.op < 0 || static_cast<size_t>(n.op) >= p.body.ops.size())
        throw std::invalid_argument("structured loop invalid operation");
      const Op& op = p.body.ops[n.op];
      if (op.n_in < 0 || op.n_in > 6)
        throw std::invalid_argument("structured loop invalid arity");
      slot(p, op.out);
      if (op.out2 >= 0) scalar(p, op.out2);
      for (int k = 0; k < op.n_in; ++k) slot(p, op.in[k]);
      if (op.opcode == OP_SET_INDEX_INPLACE ||
          op.opcode == OP_SET_SLICE_INPLACE ||
          op.opcode == OP_SET_SLICE_STRIDED_INPLACE || op.opcode == OP_ISLAND ||
          op.opcode == OP_LOOP)
        throw std::invalid_argument("unsupported structured body operation");
      const Kernel* k = find_kernel(op.opcode);
      if (!k)
        throw std::invalid_argument("unregistered structured body kernel");
      if (k->make_state)
        throw std::invalid_argument(
            "stateful structured body kernel is unsupported");
      n.forward = k->forward;
      n.backward = k->backward;
      n.kernel_scratch =
          k->scratch_size ? k->scratch_size(op, p.body.slots.data()) : 0;
      n.frame_size = add(6, p.body.slots[op.out].len);
      if (op.out2 >= 0)
        n.frame_size = add(n.frame_size, p.body.slots[op.out2].len);
      n.frame_size = add(n.frame_size, n.kernel_scratch);
      break;
    }
    case Node::Alias:
      slot(p, n.dst);
      slot(p, n.src);
      if (p.body.slots[n.dst].len != p.body.slots[n.src].len)
        throw std::invalid_argument("structured assignment changes shape");
      break;
    case Node::If:
      scalar(p, n.condition);
      if (n.children.size() != 2)
        throw std::invalid_argument("structured branch needs two arms");
      n.frame_size =
          add(1, std::max(n.children[0].frame_size, n.children[1].frame_size));
      n.target_capacity = std::max(n.children[0].target_capacity,
                                   n.children[1].target_capacity);
      break;
    case Node::For:
      scalar(p, n.lower);
      scalar(p, n.upper);
      scalar(p, n.iterator);
      if (n.children.size() != 1 || n.capacity < 0)
        throw std::invalid_argument("invalid structured for");
      n.frame_size = add(1, mul(n.capacity, add(1, n.children[0].frame_size)));
      n.target_capacity = mul(n.capacity, n.children[0].target_capacity);
      break;
    case Node::While:
      scalar(p, n.condition);
      if (n.children.size() != 2 || n.capacity < 0)
        throw std::invalid_argument("invalid structured while");
      n.frame_size = add(2, add(mul(n.capacity, add(n.children[0].frame_size,
                                                    n.children[1].frame_size)),
                                n.children[0].frame_size));
      n.target_capacity =
          add(mul(add(n.capacity, 1), n.children[0].target_capacity),
              mul(n.capacity, n.children[1].target_capacity));
      break;
    case Node::Break:
    case Node::Continue:
      if (!loop_depth) throw std::invalid_argument("unbound structured exit");
      break;
    case Node::Target:
      scalar(p, n.src);
      n.target_capacity = 1;
      break;
  }
}

struct StructuredSlotUses {
  uint64_t producers = 0;
  uint64_t kernel_inputs = 0;
  uint64_t alias_sources = 0;
  uint64_t alias_destinations = 0;
  uint64_t control = 0;
  uint64_t targets = 0;
  uint64_t outputs = 0;
  uint64_t imports = 0;
};

// H2A is deliberately narrower than ordinary indexed assignment support. It
// recognizes a functional update whose result is immediately installed in one
// stable binding cell. Ordinary kernel/control reads of the cell are
// synchronous; reverse sees their historical values after LIFO undo. A reached
// alias from the binding invalidates its mutable anchor, so the next update
// takes the ordinary copying path before compact mutation can resume. Anything
// less explicit keeps the immutable copying kernel.
void classify_compact_updates(StructuredLoop& p) {
  p.compact_update_sites = 0;
  std::vector<StructuredSlotUses> uses(p.body.slots.size());
  std::vector<uint64_t> kernel_nodes(p.body.ops.size());
  for (const auto& op : p.body.ops) {
    ++uses[static_cast<size_t>(op.out)].producers;
    if (op.out2 >= 0) ++uses[static_cast<size_t>(op.out2)].producers;
    for (int k = 0; k < op.n_in; ++k)
      ++uses[static_cast<size_t>(op.in[k])].kernel_inputs;
  }
  for (const auto& in : p.imports) ++uses[static_cast<size_t>(in.slot)].imports;
  for (int out : p.outputs) ++uses[static_cast<size_t>(out)].outputs;

  std::function<void(Node&)> inventory = [&](Node& n) {
    n.compact_update_cell = -1;
    switch (n.kind) {
      case Node::KernelCall:
        ++kernel_nodes[static_cast<size_t>(n.op)];
        break;
      case Node::Alias:
        ++uses[static_cast<size_t>(n.src)].alias_sources;
        ++uses[static_cast<size_t>(n.dst)].alias_destinations;
        break;
      case Node::If:
        ++uses[static_cast<size_t>(n.condition)].control;
        break;
      case Node::For:
        ++uses[static_cast<size_t>(n.lower)].control;
        ++uses[static_cast<size_t>(n.upper)].control;
        ++uses[static_cast<size_t>(n.iterator)].control;
        break;
      case Node::While:
        ++uses[static_cast<size_t>(n.condition)].control;
        break;
      case Node::Target:
        ++uses[static_cast<size_t>(n.src)].targets;
        break;
      default:
        break;
    }
    for (auto& child : n.children) inventory(child);
  };
  inventory(p.root);

  std::function<void(Node&)> classify = [&](Node& n) {
    if (n.kind == Node::Sequence) {
      for (size_t i = 0; i + 1 < n.children.size(); ++i) {
        Node& call = n.children[i];
        const Node& install = n.children[i + 1];
        if (call.kind != Node::KernelCall || install.kind != Node::Alias)
          continue;
        const Op& op = p.body.ops[static_cast<size_t>(call.op)];
        if (op.opcode != OP_SET_INDEX_DYNAMIC) continue;
        if (install.src != op.out || install.dst != op.in[0]) continue;
        if (op.out2 >= 0 || call.forward != set_index_forward ||
            call.backward != set_index_backward)
          continue;
        const auto* spec = static_cast<const DynamicIndexSpec*>(op.udata);
        IndexInputLayout layout;
        if (!spec || !index_input_layout(*spec, true, layout) ||
            op.n_in != layout.expected || spec->axes.empty() ||
            spec->selected_size < 0 ||
            !index_selection_is_ordered_unique(*spec) ||
            call.kernel_scratch != 0)
          continue;
        bool invalid_alias = op.out == op.in[0];
        for (int k = 1; k < op.n_in; ++k)
          invalid_alias |=
              op.in[k] == op.in[0] || op.in[k] == op.out ||
              (k < layout.selector_end && op.in[k] == op.in[layout.rhs]);
        if (invalid_alias) continue;
        const int cell = install.dst;
        if (p.body.slots[static_cast<size_t>(op.out)].len == 0 ||
            p.body.slots[static_cast<size_t>(op.out)].len !=
                p.body.slots[static_cast<size_t>(cell)].len ||
            p.body.slots[static_cast<size_t>(op.in[layout.rhs])].len !=
                spec->selected_size)
          continue;
        const auto& output = uses[static_cast<size_t>(op.out)];
        const auto& binding = uses[static_cast<size_t>(cell)];
        if (kernel_nodes[static_cast<size_t>(call.op)] != 1 ||
            output.producers != 1 || output.kernel_inputs != 0 ||
            output.alias_sources != 1 || output.alias_destinations != 0 ||
            output.control != 0 || output.targets != 0 || output.outputs != 0 ||
            output.imports != 0 || binding.producers != 0 ||
            binding.targets != 0)
          continue;
        call.compact_update_cell = cell;
        ++p.compact_update_sites;
      }
    }
    for (auto& child : n.children) classify(child);
  };
  classify(p.root);
}

// Static proof for direct control-result reuse. A result is
// eligible only when one scalar comparison produces one immediately consumed
// if/while condition and its handle cannot escape through any other graph or
// control boundary. Retained metadata is body-sized, never trip-count-sized;
// the runtime guard still declines a reached call whose canonical result would
// overlap one of its current inputs.
struct DirectControlPlan {
  enum Control : uint8_t { IfControl = 1, WhileControl = 2 };
  struct Site {
    const Node* node = nullptr;
    int op = -1;
    double canonical_handle = -1;
  };
  struct Uses {
    uint64_t producers = 0;
    uint64_t kernel_inputs = 0;
    uint64_t alias_sources = 0;
    uint64_t alias_destinations = 0;
    uint64_t if_conditions = 0;
    uint64_t while_conditions = 0;
    uint64_t for_control = 0;
    uint64_t targets = 0;
    uint64_t outputs = 0;
    uint64_t imports = 0;
  };
  struct Relation {
    const Node* producer = nullptr;
    Control control = IfControl;
    bool exits = false;
    bool effects = false;
    bool ambiguous = false;
    bool aliases = false;
  };

  std::vector<int32_t> site_by_op;
  std::vector<Site> sites;

  explicit DirectControlPlan(const StructuredLoop& p)
      : site_by_op(p.body.ops.size(), int32_t{-1}) {
    std::vector<Uses> uses(p.body.slots.size());
    std::vector<const Node*> candidates;
    std::vector<Relation> relations;

    std::function<void(const Node&)> inventory = [&](const Node& n) {
      switch (n.kind) {
        case Node::KernelCall: {
          if (n.op < 0 || static_cast<size_t>(n.op) >= p.body.ops.size())
            throw std::logic_error(
                "direct-control census operation is out of range");
          const Op& op = p.body.ops[static_cast<size_t>(n.op)];
          ++uses.at(static_cast<size_t>(op.out)).producers;
          if (op.out2 >= 0) ++uses.at(static_cast<size_t>(op.out2)).producers;
          for (int k = 0; k < op.n_in; ++k)
            ++uses.at(static_cast<size_t>(op.in[k])).kernel_inputs;
          if (op.opcode == OP_COMPARE) candidates.push_back(&n);
          break;
        }
        case Node::Alias:
          ++uses.at(static_cast<size_t>(n.src)).alias_sources;
          ++uses.at(static_cast<size_t>(n.dst)).alias_destinations;
          break;
        case Node::If:
          ++uses.at(static_cast<size_t>(n.condition)).if_conditions;
          break;
        case Node::For:
          ++uses.at(static_cast<size_t>(n.lower)).for_control;
          ++uses.at(static_cast<size_t>(n.upper)).for_control;
          ++uses.at(static_cast<size_t>(n.iterator)).for_control;
          break;
        case Node::While:
          ++uses.at(static_cast<size_t>(n.condition)).while_conditions;
          break;
        case Node::Target:
          ++uses.at(static_cast<size_t>(n.src)).targets;
          break;
        default:
          break;
      }
      for (const auto& child : n.children) inventory(child);
    };
    inventory(p.root);
    for (int output : p.outputs) ++uses.at(static_cast<size_t>(output)).outputs;
    for (const auto& import : p.imports)
      ++uses.at(static_cast<size_t>(import.slot)).imports;

    const auto condition_flags = [&](const auto& self, const Node& n,
                                     Relation& relation) -> void {
      switch (n.kind) {
        case Node::KernelCall:
          if (n.op < 0 || static_cast<size_t>(n.op) >= p.body.ops.size()) {
            relation.ambiguous = true;
          } else if (is_effectful_op(
                         p.body.ops[static_cast<size_t>(n.op)].opcode)) {
            relation.effects = true;
          }
          break;
        case Node::Alias:
          relation.aliases = true;
          break;
        case Node::Break:
        case Node::Continue:
          relation.exits = true;
          break;
        case Node::If:
        case Node::For:
        case Node::While:
          relation.ambiguous = true;
          break;
        case Node::Target:
          relation.effects = true;
          break;
        case Node::Sequence:
          break;
      }
      for (const auto& child : n.children) self(self, child, relation);
    };
    const auto terminal = [&](const auto& self, const Node& n) -> const Node* {
      if (n.kind == Node::KernelCall) return &n;
      if (n.kind != Node::Sequence || n.children.empty()) return nullptr;
      return self(self, n.children.back());
    };
    std::function<void(const Node&)> connect = [&](const Node& n) {
      if (n.kind == Node::Sequence) {
        for (size_t i = 0; i + 1 < n.children.size(); ++i) {
          const Node& producer = n.children[i];
          const Node& consumer = n.children[i + 1];
          if (producer.kind == Node::KernelCall && consumer.kind == Node::If &&
              producer.op >= 0 &&
              static_cast<size_t>(producer.op) < p.body.ops.size() &&
              p.body.ops[static_cast<size_t>(producer.op)].out ==
                  consumer.condition)
            relations.push_back({&producer, IfControl});
        }
      } else if (n.kind == Node::While && n.children.size() == 2) {
        Relation relation;
        relation.control = WhileControl;
        condition_flags(condition_flags, n.children[0], relation);
        relation.producer = terminal(terminal, n.children[0]);
        if (relation.producer && relation.producer->kind == Node::KernelCall &&
            relation.producer->op >= 0 &&
            static_cast<size_t>(relation.producer->op) < p.body.ops.size() &&
            p.body.ops[static_cast<size_t>(relation.producer->op)].out ==
                n.condition)
          relations.push_back(relation);
      }
      for (const auto& child : n.children) connect(child);
    };
    connect(p.root);

    for (const Node* candidate : candidates) {
      const Op& op = p.body.ops[static_cast<size_t>(candidate->op)];
      const Uses& output = uses.at(static_cast<size_t>(op.out));
      if ((op.n_in != 1 && op.n_in != 2) || op.out2 >= 0 ||
          p.body.slots.at(static_cast<size_t>(op.out)).len != 1 ||
          candidate->kernel_scratch != 0 || candidate->backward != nullptr ||
          !candidate->forward) {
        continue;
      }
      bool scalar_inputs = true;
      for (int k = 0; k < op.n_in; ++k)
        scalar_inputs &=
            p.body.slots.at(static_cast<size_t>(op.in[k])).len == 1;
      if (!scalar_inputs) {
        continue;
      }
      if (output.producers != 1) {
        continue;
      }
      if (output.kernel_inputs != 0) {
        continue;
      }
      if (output.alias_sources != 0 || output.alias_destinations != 0) {
        continue;
      }
      if (output.targets != 0) {
        continue;
      }
      if (output.outputs != 0) {
        continue;
      }
      if (output.imports != 0) {
        continue;
      }
      if (output.for_control != 0) {
        continue;
      }
      if (output.if_conditions + output.while_conditions != 1) {
        continue;
      }
      const Relation* direct = nullptr;
      uint64_t direct_count = 0;
      for (const auto& relation : relations)
        if (relation.producer == candidate) {
          direct = &relation;
          ++direct_count;
        }
      const Control wanted = output.if_conditions ? IfControl : WhileControl;
      if (direct_count != 1 || !direct || direct->control != wanted) {
        continue;
      }
      if (direct->exits) {
        continue;
      }
      if (direct->effects) {
        continue;
      }
      if (direct->ambiguous || direct->aliases) {
        continue;
      }
      if (site_by_op[static_cast<size_t>(candidate->op)] >= 0) {
        continue;
      }
      site_by_op[static_cast<size_t>(candidate->op)] =
          static_cast<int32_t>(sites.size());
      sites.push_back({candidate, candidate->op});
    }
  }
};

// Prove body operations invariant for a retained for/while scope without
// moving them across their first reached source position. The first execution
// remains lazy; later reaches may reuse an inactive result only when every
// input handle is identical. This preserves zero-trip loops, untaken branches,
// exception order, and all active reverse history.
struct LoopInvariantPlan {
  struct Loop {
    const Node* node = nullptr;
    uint64_t generation = 0;
  };
  struct LoopIndex {
    const Node* node = nullptr;
    uint32_t loop = 0;
  };
  struct Site {
    const Node* node = nullptr;
    int op = -1;
    uint32_t loop = 0;
    std::array<double, 6> cached_inputs{};
    double cached_out = -1;
    double cached_out2 = -1;
    uint64_t cached_generation = 0;
  };

  std::vector<int32_t> site_by_op;
  std::vector<Loop> loops;
  std::vector<LoopIndex> loop_index;
  std::vector<Site> sites;

  static bool pure_candidate(const StructuredLoop& p, const Node& n) {
    if (n.kind != Node::KernelCall || n.op < 0 ||
        static_cast<size_t>(n.op) >= p.body.ops.size() || !n.forward ||
        n.compact_update_cell >= 0)
      return false;
    const uint16_t opcode = p.body.ops[static_cast<size_t>(n.op)].opcode;
    return !is_effectful_op(opcode) && opcode != OP_ISLAND && opcode != OP_LOOP;
  }

  explicit LoopInvariantPlan(const StructuredLoop& p)
      : site_by_op(p.body.ops.size(), int32_t{-1}) {
    const size_t slot_count = p.body.slots.size();
    const size_t op_count = p.body.ops.size();
    std::vector<uint32_t> op_nodes(op_count, 0);
    std::vector<const Node*> node_by_op(op_count, nullptr);
    std::vector<uint8_t> inside_loop(op_count, 0);
    std::vector<size_t> parent(slot_count);
    std::iota(parent.begin(), parent.end(), size_t{0});
    const auto root = [&](size_t slot) {
      size_t result = slot;
      while (parent[result] != result) result = parent[result];
      while (parent[slot] != slot) {
        const size_t next = parent[slot];
        parent[slot] = result;
        slot = next;
      }
      return result;
    };
    const auto unite = [&](size_t a, size_t b) {
      const size_t ar = root(a), br = root(b);
      if (ar != br) parent[br] = ar;
    };

    std::function<void(const Node&, uint32_t)> inventory =
        [&](const Node& n, uint32_t loop_depth) {
          if (n.kind == Node::KernelCall) {
            if (n.op < 0 || static_cast<size_t>(n.op) >= op_count)
              throw std::logic_error(
                  "loop-invariant operation is out of range");
            ++op_nodes[static_cast<size_t>(n.op)];
            node_by_op[static_cast<size_t>(n.op)] = &n;
            if (loop_depth) inside_loop[static_cast<size_t>(n.op)] = 1;
          } else if (n.kind == Node::Alias) {
            if (n.src < 0 || n.dst < 0 ||
                static_cast<size_t>(n.src) >= slot_count ||
                static_cast<size_t>(n.dst) >= slot_count)
              throw std::logic_error("loop-invariant alias is out of range");
            unite(static_cast<size_t>(n.src), static_cast<size_t>(n.dst));
          } else if (n.kind == Node::For || n.kind == Node::While) {
            if (loops.size() >= std::numeric_limits<uint32_t>::max())
              throw std::length_error("too many structured loops");
            loops.push_back({&n});
          }
          const uint32_t child_depth =
              loop_depth + (n.kind == Node::For || n.kind == Node::While);
          for (const auto& child : n.children) inventory(child, child_depth);
        };
    inventory(p.root, 0);

    loop_index.reserve(loops.size());
    for (uint32_t i = 0; i < loops.size(); ++i)
      loop_index.push_back({loops[i].node, i});
    std::sort(loop_index.begin(), loop_index.end(),
              [](const LoopIndex& a, const LoopIndex& b) {
                return std::less<const Node*>{}(a.node, b.node);
              });

    std::vector<uint8_t> admissible(op_count, 0);
    for (size_t op = 0; op < op_count; ++op)
      if (inside_loop[op] && op_nodes[op] == 1 &&
          pure_candidate(p, *node_by_op[op]))
        admissible[op] = 1;

    // Preorder places an enclosing loop before its nested loops, so the first
    // successful assignment gives each site its widest proved safe scope.
    for (uint32_t loop_id = 0; loop_id < loops.size(); ++loop_id) {
      const Node& loop = *loops[loop_id].node;
      std::vector<uint32_t> writers(slot_count, 0);
      std::vector<uint8_t> contained(op_count, 0);
      std::vector<uint8_t> compact_roots(slot_count, 0);
      const auto collect = [&](const auto& self, const Node& n) -> void {
        switch (n.kind) {
          case Node::KernelCall: {
            const Op& op = p.body.ops.at(static_cast<size_t>(n.op));
            ++writers.at(static_cast<size_t>(op.out));
            if (op.out2 >= 0) ++writers.at(static_cast<size_t>(op.out2));
            contained.at(static_cast<size_t>(n.op)) = 1;
            if (n.compact_update_cell >= 0) {
              compact_roots.at(
                  root(static_cast<size_t>(n.compact_update_cell))) = 1;
              if (op.n_in > 0)
                compact_roots.at(root(static_cast<size_t>(op.in[0]))) = 1;
            }
            break;
          }
          case Node::Alias:
            ++writers.at(static_cast<size_t>(n.dst));
            break;
          case Node::For:
            ++writers.at(static_cast<size_t>(n.iterator));
            break;
          default:
            break;
        }
        for (const auto& child : n.children) self(self, child);
      };
      collect(collect, loop);

      std::vector<uint8_t> invariant(slot_count, 0);
      for (size_t slot = 0; slot < slot_count; ++slot)
        invariant[slot] = writers[slot] == 0 && !compact_roots[root(slot)];
      bool changed = true;
      while (changed) {
        changed = false;
        for (size_t op_id = 0; op_id < op_count; ++op_id) {
          if (!contained[op_id] || !admissible[op_id]) continue;
          const Op& op = p.body.ops[op_id];
          if (writers[static_cast<size_t>(op.out)] != 1 ||
              compact_roots[root(static_cast<size_t>(op.out))] ||
              (op.out2 >= 0 &&
               (writers[static_cast<size_t>(op.out2)] != 1 ||
                compact_roots[root(static_cast<size_t>(op.out2))])))
            continue;
          bool inputs_invariant = true;
          for (int k = 0; k < op.n_in; ++k)
            inputs_invariant &= invariant[static_cast<size_t>(op.in[k])] != 0;
          if (!inputs_invariant) continue;
          if (!invariant[static_cast<size_t>(op.out)]) {
            invariant[static_cast<size_t>(op.out)] = 1;
            changed = true;
          }
          if (op.out2 >= 0 && !invariant[static_cast<size_t>(op.out2)]) {
            invariant[static_cast<size_t>(op.out2)] = 1;
            changed = true;
          }
        }
      }

      for (size_t op_id = 0; op_id < op_count; ++op_id) {
        if (!contained[op_id] || !admissible[op_id] || site_by_op[op_id] >= 0)
          continue;
        const Op& op = p.body.ops[op_id];
        if (writers[static_cast<size_t>(op.out)] != 1 ||
            !invariant[static_cast<size_t>(op.out)] ||
            (op.out2 >= 0 && (writers[static_cast<size_t>(op.out2)] != 1 ||
                              !invariant[static_cast<size_t>(op.out2)])))
          continue;
        bool inputs_invariant = true;
        for (int k = 0; k < op.n_in; ++k)
          inputs_invariant &= invariant[static_cast<size_t>(op.in[k])] != 0;
        if (!inputs_invariant) continue;
        site_by_op[op_id] = static_cast<int32_t>(sites.size());
        sites.push_back({node_by_op[op_id], static_cast<int>(op_id), loop_id});
      }
    }
  }

  int32_t find_loop(const Node& node) const noexcept {
    const auto at =
        std::lower_bound(loop_index.begin(), loop_index.end(), &node,
                         [](const LoopIndex& entry, const Node* wanted) {
                           return std::less<const Node*>{}(entry.node, wanted);
                         });
    return at != loop_index.end() && at->node == &node
               ? static_cast<int32_t>(at->loop)
               : int32_t{-1};
  }

  void reset_runtime() noexcept {
    for (auto& loop : loops) loop.generation = 0;
    for (auto& site : sites) {
      site.cached_inputs.fill(0.0);
      site.cached_out = site.cached_out2 = -1;
      site.cached_generation = 0;
    }
  }
};

// Find contiguous pure kernel chains whose only observable result is an
// immediately following if/while condition. At runtime the whole cone is
// admitted atomically only when all external handles are inactive. Its
// callbacks still run in source order, but their temporary values live in one
// body-sized canonical buffer instead of per-iteration history.
struct InactiveControlPlan {
  struct Uses {
    uint64_t producers = 0;
    uint64_t aliases = 0;
    uint64_t controls = 0;
    uint64_t targets = 0;
    uint64_t outputs = 0;
    uint64_t imports = 0;
    std::vector<int> kernel_consumers;
  };
  struct Site {
    const Node* node = nullptr;
    int op = -1;
    uint32_t cone = 0;
    bool first = false;
    bool last = false;
    uint64_t value_offset = 0;
    double canonical_handle = -1;
  };
  struct Cone {
    enum Decision : uint8_t { Direct, Ordinary } decision = Ordinary;
    std::vector<int> external_slots;
    uint32_t first_site = 0;
    uint32_t site_count = 0;
    uint64_t value_offset = 0;
    uint64_t value_count = 0;
    bool running = false;
  };

  std::vector<int32_t> site_by_op;
  std::vector<Site> sites;
  std::vector<Cone> cones;
  uint64_t value_count = 0;

  explicit InactiveControlPlan(const StructuredLoop& p)
      : site_by_op(p.body.ops.size(), int32_t{-1}) {
    const size_t op_count = p.body.ops.size();
    std::vector<Uses> uses(p.body.slots.size());
    std::vector<uint32_t> op_nodes(op_count, 0);
    std::vector<const Node*> node_by_op(op_count, nullptr);
    std::vector<uint8_t> inside_loop(op_count, 0);
    std::function<void(const Node&, uint32_t)> inventory =
        [&](const Node& n, uint32_t loop_depth) {
          switch (n.kind) {
            case Node::KernelCall: {
              if (n.op < 0 || static_cast<size_t>(n.op) >= op_count)
                throw std::logic_error(
                    "inactive-control operation is out of range");
              const Op& op = p.body.ops[static_cast<size_t>(n.op)];
              ++op_nodes[static_cast<size_t>(n.op)];
              node_by_op[static_cast<size_t>(n.op)] = &n;
              ++uses.at(static_cast<size_t>(op.out)).producers;
              if (op.out2 >= 0)
                ++uses.at(static_cast<size_t>(op.out2)).producers;
              for (int k = 0; k < op.n_in; ++k)
                uses.at(static_cast<size_t>(op.in[k]))
                    .kernel_consumers.push_back(n.op);
              if (loop_depth) inside_loop[static_cast<size_t>(n.op)] = 1;
              break;
            }
            case Node::Alias:
              ++uses.at(static_cast<size_t>(n.src)).aliases;
              ++uses.at(static_cast<size_t>(n.dst)).aliases;
              break;
            case Node::If:
              ++uses.at(static_cast<size_t>(n.condition)).controls;
              break;
            case Node::For:
              ++uses.at(static_cast<size_t>(n.lower)).controls;
              ++uses.at(static_cast<size_t>(n.upper)).controls;
              ++uses.at(static_cast<size_t>(n.iterator)).aliases;
              break;
            case Node::While:
              ++uses.at(static_cast<size_t>(n.condition)).controls;
              break;
            case Node::Target:
              ++uses.at(static_cast<size_t>(n.src)).targets;
              break;
            default:
              break;
          }
          const uint32_t child_depth =
              loop_depth + (n.kind == Node::For || n.kind == Node::While);
          for (const auto& child : n.children) inventory(child, child_depth);
        };
    inventory(p.root, 0);
    for (int output : p.outputs) ++uses.at(static_cast<size_t>(output)).outputs;
    for (const auto& import : p.imports)
      ++uses.at(static_cast<size_t>(import.slot)).imports;

    std::vector<uint8_t> candidate(op_count, 0), admitted(op_count, 0);
    for (size_t op_id = 0; op_id < op_count; ++op_id) {
      if (!inside_loop[op_id] || op_nodes[op_id] != 1) continue;
      const Node& node = *node_by_op[op_id];
      const Op& op = p.body.ops[op_id];
      if (!LoopInvariantPlan::pure_candidate(p, node) || op.out2 >= 0 ||
          node.kernel_scratch != 0 || node.compact_update_cell >= 0)
        continue;
      candidate[op_id] = 1;
    }

    bool changed = true;
    while (changed) {
      changed = false;
      for (size_t op_id = 0; op_id < op_count; ++op_id) {
        if (!candidate[op_id] || admitted[op_id]) continue;
        const Op& op = p.body.ops[op_id];
        const Uses& output = uses.at(static_cast<size_t>(op.out));
        if (output.producers != 1 || output.aliases || output.targets ||
            output.outputs || output.imports)
          continue;
        bool all_consumers_admitted = true;
        bool reaches_control = output.controls != 0;
        for (int consumer : output.kernel_consumers) {
          if (consumer < 0 || static_cast<size_t>(consumer) >= op_count ||
              !admitted[static_cast<size_t>(consumer)]) {
            all_consumers_admitted = false;
            break;
          }
          reaches_control = true;
        }
        if (!all_consumers_admitted || !reaches_control) continue;
        admitted[op_id] = 1;
        changed = true;
      }
    }

    const auto add_cone = [&](const std::vector<const Node*>& run,
                              int condition) {
      if (run.empty() || run.back()->kind != Node::KernelCall) return;
      const Op& terminal = p.body.ops.at(static_cast<size_t>(run.back()->op));
      if (terminal.out != condition) return;
      std::vector<uint8_t> needed(p.body.slots.size(), 0);
      std::vector<uint8_t> selected(run.size(), 0);
      needed.at(static_cast<size_t>(condition)) = 1;
      for (size_t i = run.size(); i-- > 0;) {
        const Node& node = *run[i];
        const Op& op = p.body.ops.at(static_cast<size_t>(node.op));
        if (!needed[static_cast<size_t>(op.out)]) continue;
        if (!admitted[static_cast<size_t>(node.op)]) return;
        selected[i] = 1;
        needed[static_cast<size_t>(op.out)] = 0;
        for (int k = 0; k < op.n_in; ++k)
          needed[static_cast<size_t>(op.in[k])] = 1;
      }
      size_t first = 0;
      while (first < selected.size() && !selected[first]) ++first;
      if (first == selected.size()) return;
      for (size_t i = first; i < selected.size(); ++i)
        if (!selected[i]) return;
      // A single comparison is already covered by DirectControlPlan. Keeping
      // only multi-operation cones avoids redundant preflight and storage.
      if (run.size() - first < 2) return;

      std::vector<uint8_t> selected_op(op_count, 0);
      for (size_t i = first; i < run.size(); ++i) {
        const size_t op_id = static_cast<size_t>(run[i]->op);
        if (site_by_op[op_id] >= 0) return;
        selected_op[op_id] = 1;
      }
      for (size_t i = first; i < run.size(); ++i) {
        const Op& op = p.body.ops[static_cast<size_t>(run[i]->op)];
        const Uses& output = uses.at(static_cast<size_t>(op.out));
        const uint64_t expected_controls = op.out == condition ? 1 : 0;
        if (output.controls != expected_controls) return;
        for (int consumer : output.kernel_consumers)
          if (consumer < 0 || static_cast<size_t>(consumer) >= op_count ||
              !selected_op[static_cast<size_t>(consumer)])
            return;
      }
      if (cones.size() >= std::numeric_limits<uint32_t>::max() ||
          sites.size() >=
              static_cast<size_t>(std::numeric_limits<int32_t>::max()) ||
          run.size() - first > std::numeric_limits<uint32_t>::max())
        throw std::length_error("inactive-control plan is too large");
      const uint32_t cone_id = static_cast<uint32_t>(cones.size());
      cones.emplace_back();
      auto& cone = cones.back();
      cone.first_site = static_cast<uint32_t>(sites.size());
      cone.site_count = static_cast<uint32_t>(run.size() - first);
      cone.value_offset = value_count;
      std::vector<uint8_t> external(p.body.slots.size(), 0);
      for (size_t i = first; i < run.size(); ++i) {
        const Op& op = p.body.ops[static_cast<size_t>(run[i]->op)];
        for (int k = 0; k < op.n_in; ++k) {
          const int input = op.in[k];
          bool produced_inside = false;
          for (size_t j = first; j < i; ++j)
            produced_inside |=
                p.body.ops[static_cast<size_t>(run[j]->op)].out == input;
          if (!produced_inside) external[static_cast<size_t>(input)] = 1;
        }
      }
      for (size_t slot = 0; slot < external.size(); ++slot)
        if (external[slot])
          cone.external_slots.push_back(static_cast<int>(slot));
      for (size_t i = first; i < run.size(); ++i) {
        const int op_id = run[i]->op;
        const int64_t len =
            p.body.slots
                .at(static_cast<size_t>(
                    p.body.ops.at(static_cast<size_t>(op_id)).out))
                .len;
        if (len < 0 || value_count > static_cast<uint64_t>(exact_limit) -
                                         static_cast<uint64_t>(len))
          throw std::length_error("inactive-control value extent overflow");
        site_by_op[static_cast<size_t>(op_id)] =
            static_cast<int32_t>(sites.size());
        sites.push_back({run[i], op_id, cone_id, i == first,
                         i + 1 == run.size(), value_count});
        value_count += static_cast<uint64_t>(len);
      }
      cone.value_count = value_count - cone.value_offset;
    };
    const auto suffix = [](const Node& node) {
      std::vector<const Node*> run;
      if (node.kind == Node::KernelCall) {
        run.push_back(&node);
      } else if (node.kind == Node::Sequence) {
        size_t first = node.children.size();
        while (first > 0 && node.children[first - 1].kind == Node::KernelCall)
          --first;
        for (size_t i = first; i < node.children.size(); ++i)
          run.push_back(&node.children[i]);
      }
      return run;
    };
    const auto discover = [&](const auto& self, const Node& node) -> void {
      if (node.kind == Node::Sequence) {
        for (size_t i = 0; i < node.children.size(); ++i) {
          const Node& control = node.children[i];
          if (control.kind != Node::If || i == 0) continue;
          size_t first = i;
          while (first > 0 && node.children[first - 1].kind == Node::KernelCall)
            --first;
          std::vector<const Node*> run;
          for (size_t j = first; j < i; ++j) run.push_back(&node.children[j]);
          add_cone(run, control.condition);
        }
      }
      if (node.kind == Node::While && node.children.size() == 2)
        add_cone(suffix(node.children[0]), node.condition);
      for (const auto& child : node.children) self(self, child);
    };
    discover(discover, p.root);
  }

  void reset_runtime() noexcept {
    for (auto& site : sites) site.canonical_handle = -1;
    for (auto& cone : cones) {
      cone.decision = Cone::Ordinary;
      cone.running = false;
    }
  }
};

// Reuse one body-sized output buffer for an inactive call when its result is
// provably forward-only.  A consumer with a reverse callback may need an
// inactive input's primal during the reverse sweep, so such inputs stay on the
// ordinary persistent arena path.  Aliases and targets also let a particular
// reached value escape its producing binding.  The remaining results have no
// lifetime beyond their current binding and synchronous forward consumers;
// reexecuting the same unique call site may therefore overwrite its buffer.
//
// Keep these sites disjoint from loop-invariant and inactive-control plans.
// Both plans use canonical handles as cached identity, while this plan
// deliberately gives every dynamic version from one site the same handle.
struct InactiveWorkspacePlan {
  struct Uses {
    uint64_t producers = 0;
    uint64_t aliases = 0;
    uint64_t targets = 0;
    std::vector<const Node*> kernel_consumers;
  };
  struct Site {
    const Node* node = nullptr;
    int op = -1;
    int64_t value_count = 0;
    std::unique_ptr<double[]> values;
    double canonical_handle = -1;
    bool canonical_ready = false;
    bool disabled = false;
  };

  std::vector<int32_t> site_by_op;
  std::vector<Site> sites;
  uint64_t value_count = 0;

  InactiveWorkspacePlan(const StructuredLoop& p,
                        const LoopInvariantPlan* invariants,
                        const InactiveControlPlan* controls)
      : site_by_op(p.body.ops.size(), int32_t{-1}) {
    const size_t op_count = p.body.ops.size();
    std::vector<Uses> uses(p.body.slots.size());
    std::vector<uint32_t> op_nodes(op_count, 0);
    std::vector<const Node*> node_by_op(op_count, nullptr);
    std::vector<uint8_t> inside_loop(op_count, 0);
    const auto inventory = [&](const auto& self, const Node& n,
                               uint32_t loop_depth) -> void {
      switch (n.kind) {
        case Node::KernelCall: {
          if (n.op < 0 || static_cast<size_t>(n.op) >= op_count)
            throw std::logic_error(
                "inactive-workspace operation is out of range");
          const Op& op = p.body.ops[static_cast<size_t>(n.op)];
          ++op_nodes[static_cast<size_t>(n.op)];
          node_by_op[static_cast<size_t>(n.op)] = &n;
          if (loop_depth) inside_loop[static_cast<size_t>(n.op)] = 1;
          ++uses.at(static_cast<size_t>(op.out)).producers;
          if (op.out2 >= 0) ++uses.at(static_cast<size_t>(op.out2)).producers;
          for (int k = 0; k < op.n_in; ++k)
            uses.at(static_cast<size_t>(op.in[k]))
                .kernel_consumers.push_back(&n);
          break;
        }
        case Node::Alias:
          ++uses.at(static_cast<size_t>(n.src)).aliases;
          ++uses.at(static_cast<size_t>(n.dst)).aliases;
          break;
        case Node::Target:
          ++uses.at(static_cast<size_t>(n.src)).targets;
          break;
        default:
          break;
      }
      const uint32_t child_depth =
          loop_depth + (n.kind == Node::For || n.kind == Node::While);
      for (const auto& child : n.children) self(self, child, child_depth);
    };
    inventory(inventory, p.root, 0);

    const auto planned_elsewhere = [&](size_t op_id) {
      return (invariants && op_id < invariants->site_by_op.size() &&
              invariants->site_by_op[op_id] >= 0) ||
             (controls && op_id < controls->site_by_op.size() &&
              controls->site_by_op[op_id] >= 0);
    };
    for (size_t op_id = 0; op_id < op_count; ++op_id) {
      if (!inside_loop[op_id] || op_nodes[op_id] != 1 ||
          planned_elsewhere(op_id))
        continue;
      const Node& node = *node_by_op[op_id];
      const Op& op = p.body.ops[op_id];
      const Kernel* kernel = find_kernel(op.opcode);
      if (!kernel || !node.forward || node.forward != kernel->forward ||
          node.backward != kernel->backward || is_effectful_op(op.opcode) ||
          op.out2 >= 0 || node.compact_update_cell >= 0)
        continue;
      const Uses& output = uses.at(static_cast<size_t>(op.out));
      if (output.producers != 1 || output.aliases != 0 || output.targets != 0)
        continue;
      bool safe_consumers = true;
      for (const Node* consumer : output.kernel_consumers) {
        if (!consumer || consumer->backward || consumer->op < 0 ||
            static_cast<size_t>(consumer->op) >= op_count ||
            planned_elsewhere(static_cast<size_t>(consumer->op))) {
          safe_consumers = false;
          break;
        }
      }
      if (!safe_consumers) continue;
      bool self_input = false;
      for (int k = 0; k < op.n_in; ++k) self_input |= op.in[k] == op.out;
      if (self_input) continue;
      const int64_t len = p.body.slots.at(static_cast<size_t>(op.out)).len;
      if (len <= 0 || static_cast<uint64_t>(len) >
                          static_cast<uint64_t>(exact_limit) - value_count)
        continue;
      if (sites.size() >=
          static_cast<size_t>(std::numeric_limits<int32_t>::max()))
        throw std::length_error("inactive-workspace plan is too large");
      site_by_op[op_id] = static_cast<int32_t>(sites.size());
      sites.push_back(
          {&node, static_cast<int>(op_id), len, nullptr, double{-1}});
      value_count += static_cast<uint64_t>(len);
    }
  }

  void reset_runtime() noexcept {
    for (auto& site : sites) {
      site.canonical_handle = -1;
      site.canonical_ready = false;
    }
  }
};

int64_t offset(double handle) {
  return static_cast<int64_t>(handle >= 0 ? handle : -handle - 1);
}
double handle(int64_t at, bool active) {
  return active ? static_cast<double>(at) : -static_cast<double>(at) - 1;
}

// Dynamic counted-loop iterators are inactive int32 scalars.  Keep their
// exact value in a disjoint handle band instead of allocating a value and a
// generic Ref for every reached iteration. Ordinary inactive Ref handles use
// the first 2^40 negative integers, an unreachable 24-TiB table ceiling. Arena
// locations use the rest of the original 2^52 band, and inline int32 values
// occupy the next 2^32 exact integers.
constexpr int64_t ordinary_ref_limit = int64_t{1} << 40;
constexpr int64_t record_scalar_first = ordinary_ref_limit;
constexpr int64_t record_scalar_count = exact_limit - record_scalar_first + 1;
constexpr int64_t inline_int_count = int64_t{1} << 32;
constexpr int64_t inline_int_first = exact_limit + 1;
constexpr int64_t inline_int_last = exact_limit + inline_int_count;
constexpr int64_t arena_location_first = ordinary_ref_limit + 1;
constexpr int64_t arena_location_count = exact_limit - arena_location_first + 1;
constexpr size_t arena_location_offset_bits = 24;
constexpr size_t arena_location_block_values = size_t{1}
                                               << arena_location_offset_bits;
constexpr uint64_t arena_location_block_count =
    static_cast<uint64_t>(arena_location_count) >> arena_location_offset_bits;
static_assert(std::numeric_limits<double>::is_iec559 &&
                  std::numeric_limits<double>::digits == 53,
              "structured handles require IEEE binary64 doubles");
static_assert(ordinary_ref_limit < arena_location_first &&
                  arena_location_first <= exact_limit &&
                  exact_limit < inline_int_first &&
                  inline_int_last <= (int64_t{1} << 53),
              "structured handle bands must be disjoint");
static_assert(record_scalar_count > 0,
              "structured record-scalar handle band must be nonempty");
static_assert(arena_location_count > 0 &&
                  (arena_location_count % arena_location_block_values) == 0,
              "arena handle range must contain complete blocks");

bool is_inline_int(double h) noexcept {
  return h < -static_cast<double>(exact_limit);
}

bool is_arena_location(double h) noexcept {
  return h < -static_cast<double>(ordinary_ref_limit) &&
         h >= -static_cast<double>(exact_limit);
}

bool is_record_scalar(double h) noexcept {
  return h >= static_cast<double>(record_scalar_first) &&
         h <= static_cast<double>(exact_limit);
}

double record_scalar_handle(uint64_t ordinal) {
  if (ordinal >= static_cast<uint64_t>(record_scalar_count))
    throw std::length_error("dynamic structured record-scalar overflow");
  return static_cast<double>(record_scalar_first) +
         static_cast<double>(ordinal);
}

double inline_int_handle(int32_t value) noexcept {
  const int64_t biased =
      static_cast<int64_t>(value) -
      static_cast<int64_t>(std::numeric_limits<int32_t>::min());
  return -static_cast<double>(inline_int_first + biased);
}

double inline_int_value(double h) noexcept {
  // Handles are executor-private and only inline_int_handle constructs this
  // band.  Decode with exact double arithmetic so the hot callback path needs
  // no floating-to-integer conversion or repeated canonical-form validation.
  return static_cast<double>(std::numeric_limits<int32_t>::min()) +
         (-h - static_cast<double>(inline_int_first));
}

struct Execution {
  const StructuredLoop& p;
  KernelCtx& outer;
  double* arena;
  double* bindings;
  int64_t targets = 0;

  explicit Execution(KernelCtx& c)
      : p(*static_cast<const StructuredLoop*>(c.udata)),
        outer(c),
        arena(c.scratch),
        bindings(arena + p.bindings_offset) {}

  double* value(int s) { return arena + offset(bindings[s]); }
  double* adj(double h) {
    if (h < 0) return nullptr;
    const int64_t at = offset(h);
    // Accumulate directly into graph inputs, preserving the interleaving
    // with contributions already present from later graph operations.
    if (at < p.initial_size) {
      for (const auto& in : p.imports) {
        const Slot& s = p.body.slots[in.slot];
        if (at >= s.offset && at - s.offset < s.len) {
          double* a = outer.in_adj[in.input].data;
          return a ? a + in.offset + at - s.offset : nullptr;
        }
      }
    }
    return arena + p.adjoint_offset + at;
  }

  KernelCtx context(const Node& n, int64_t frame, bool backward) {
    const Op& op = p.body.ops[n.op];
    KernelCtx c{};
    c.n_in = op.n_in;
    c.variant = op.variant;
    c.idata = op.idata;
    c.n_idata = op.n_idata;
    c.udata = op.udata;
    c.eval_state = outer.eval_state;
    for (int k = 0; k < op.n_in; ++k) {
      const double h = backward ? arena[frame + k] : bindings[op.in[k]];
      c.in[k] = {arena + offset(h), p.body.slots[op.in[k]].len};
      c.in_adj[k] = {adj(h), c.in[k].len};
      if (!backward) arena[frame + k] = h;
    }
    int64_t pos = frame + 6;
    c.out = {arena + pos, p.body.slots[op.out].len};
    c.out_adj_vec = {arena + p.adjoint_offset + pos, c.out.len};
    if (backward && c.out.len == 1) c.out_adj = c.out_adj_vec.data[0];
    pos += c.out.len;
    if (op.out2 >= 0) {
      c.out2 = {arena + pos, p.body.slots[op.out2].len};
      if (backward) c.out2_adj = arena[p.adjoint_offset + pos];
      pos += c.out2.len;
    }
    c.scratch = arena + pos;
    return c;
  }

  enum Flow { Normal, Break, Continue };
  Flow forward(const Node& n, int64_t frame) {
    switch (n.kind) {
      case Node::Sequence: {
        arena[frame] = 0;
        int64_t pos = frame + 1;
        for (size_t i = 0; i < n.children.size(); ++i) {
          arena[frame] = static_cast<double>(i + 1);
          const Flow f = forward(n.children[i], pos);
          if (f != Normal) return f;
          pos += n.children[i].frame_size;
        }
        return Normal;
      }
      case Node::KernelCall: {
        KernelCtx c = context(n, frame, false);
        n.forward(c);
        bool active = false;
        for (int k = 0; k < c.n_in; ++k) active |= arena[frame + k] >= 0;
        active &= n.backward != nullptr;
        const Op& op = p.body.ops[n.op];
        bindings[op.out] = handle(frame + 6, active);
        if (op.out2 >= 0)
          bindings[op.out2] = handle(frame + 6 + c.out.len, active);
        return Normal;
      }
      case Node::Alias:
        bindings[n.dst] = bindings[n.src];
        return Normal;
      case Node::If: {
        const int arm = value(n.condition)[0] != 0.0 ? 0 : 1;
        arena[frame] = arm;
        return forward(n.children[arm], frame + 1);
      }
      case Node::For: {
        const double lo = value(n.lower)[0], hi = value(n.upper)[0];
        if (!std::isfinite(lo) || !std::isfinite(hi) || std::trunc(lo) != lo ||
            std::trunc(hi) != hi || lo < std::numeric_limits<int32_t>::min() ||
            hi < std::numeric_limits<int32_t>::min() ||
            lo > std::numeric_limits<int32_t>::max() ||
            hi > std::numeric_limits<int32_t>::max())
          throw std::logic_error("structured loop invalid integer bounds");
        const int64_t count = hi >= lo ? static_cast<int64_t>(hi - lo) + 1 : 0;
        if (count > n.capacity)
          throw std::logic_error("structured loop capacity proof failed");
        arena[frame] = 0;
        const int64_t stride = 1 + n.children[0].frame_size;
        for (int64_t i = 0; i < count; ++i) {
          const int64_t pos = frame + 1 + i * stride;
          arena[pos] = lo + static_cast<double>(i);
          bindings[n.iterator] = handle(pos, false);
          arena[frame] = static_cast<double>(i + 1);
          if (forward(n.children[0], pos + 1) == Break) break;
        }
        return Normal;
      }
      case Node::While: {
        arena[frame] = arena[frame + 1] = 0;
        const int64_t stride =
            n.children[0].frame_size + n.children[1].frame_size;
        for (int64_t i = 0;; ++i) {
          const int64_t pos = frame + 2 + i * stride;
          forward(n.children[0], pos);
          if (value(n.condition)[0] == 0.0) {
            arena[frame + 1] = 1;
            break;
          }
          if (i == n.capacity)
            throw std::logic_error("structured while capacity proof failed");
          arena[frame] = static_cast<double>(i + 1);
          if (forward(n.children[1], pos + n.children[0].frame_size) == Break)
            break;
        }
        return Normal;
      }
      case Node::Break:
        return Break;
      case Node::Continue:
        return Continue;
      case Node::Target:
        if (targets >= p.root.target_capacity)
          throw std::logic_error("structured target capacity proof failed");
        arena[p.target_refs_offset + targets++] = bindings[n.src];
        return Normal;
    }
    throw std::logic_error("invalid structured node");
  }

  void backward(const Node& n, int64_t frame) {
    switch (n.kind) {
      case Node::Sequence: {
        const size_t count = static_cast<size_t>(arena[frame]);
        int64_t pos = frame + 1;
        for (size_t i = 0; i < count; ++i) pos += n.children[i].frame_size;
        for (size_t i = count; i-- > 0;) {
          pos -= n.children[i].frame_size;
          backward(n.children[i], pos);
        }
        return;
      }
      case Node::KernelCall:
        if (n.backward) {
          KernelCtx c = context(n, frame, true);
          n.backward(c);
        }
        return;
      case Node::If:
        backward(n.children[static_cast<size_t>(arena[frame])], frame + 1);
        return;
      case Node::For: {
        const int64_t stride = 1 + n.children[0].frame_size;
        for (int64_t i = static_cast<int64_t>(arena[frame]); i-- > 0;)
          backward(n.children[0], frame + 2 + i * stride);
        return;
      }
      case Node::While: {
        const int64_t count = static_cast<int64_t>(arena[frame]);
        const int64_t stride =
            n.children[0].frame_size + n.children[1].frame_size;
        if (arena[frame + 1] != 0)
          backward(n.children[0], frame + 2 + count * stride);
        for (int64_t i = count; i-- > 0;) {
          const int64_t pos = frame + 2 + i * stride;
          backward(n.children[1], pos + n.children[0].frame_size);
          backward(n.children[0], pos);
        }
        return;
      }
      default:
        return;
    }
  }
};

// Packed native tape for plans whose rectangular maximum history cannot be
// preallocated. Each reached kernel contributes exactly one fixed-size frame;
// control is represented by the order of those frames, so untaken arms and
// unused loop capacity consume no numerical history. Blocks keep every frame
// address stable without a geometrically over-allocated contiguous vector.
struct DynamicArena {
  struct Allocation {
    double* data = nullptr;
    size_t block = 0;
    size_t offset = 0;
    size_t count = 0;
  };
  struct Block {
    std::unique_ptr<double[]> data;
    size_t capacity = 0;
    size_t used = 0;
  };
  std::vector<Block> blocks;
  size_t next_capacity = 1 << 20;  // eight MiB initially
  uint64_t used_values = 0;
  uint64_t capacity_values = 0;

  void clear() {
    std::vector<Block>{}.swap(blocks);
    next_capacity = 1 << 20;
    used_values = capacity_values = 0;
  }

  Allocation allocate_located(int64_t count) {
    if (count < 0 ||
        static_cast<uint64_t>(count) > std::numeric_limits<size_t>::max())
      throw std::length_error("dynamic structured history overflow");
    const size_t n = static_cast<size_t>(count);
    if (n == 0) return {};
    if (blocks.empty() || n > blocks.back().capacity - blocks.back().used) {
      const size_t capacity = std::max(n, next_capacity);
      if (capacity > std::numeric_limits<uint64_t>::max() - capacity_values)
        throw std::length_error("dynamic structured history overflow");
      // Every allocated range is initialized by its owner before publication.
      // Avoid value-initializing unused capacity in production: touching a
      // geometrically grown block here makes its entire reserved tail
      // resident. Debug builds poison blocks so the focused structured-loop
      // suite exposes any accidental dependence on fresh zeroes.
      std::unique_ptr<double[]> data(new double[capacity]);
#ifndef NDEBUG
      std::fill_n(data.get(), capacity,
                  std::numeric_limits<double>::quiet_NaN());
#endif
      blocks.push_back({std::move(data), capacity, 0});
      capacity_values += capacity;
      if (next_capacity < arena_location_block_values)
        next_capacity =
            std::min(arena_location_block_values, next_capacity * 2);
    }
    const size_t block_index = blocks.size() - 1;
    Block& block = blocks.back();
    if (n > std::numeric_limits<uint64_t>::max() - used_values)
      throw std::length_error("dynamic structured history overflow");
    const size_t block_offset = block.used;
    double* result = block.data.get() + block_offset;
    block.used += n;
    used_values += n;
    return {result, block_index, block_offset, n};
  }

  double* allocate(int64_t count) { return allocate_located(count).data; }

  bool location_handle(const Allocation& allocation, size_t relative,
                       int64_t len, double& result) const noexcept {
    if (len <= 0 || allocation.block >= arena_location_block_count ||
        allocation.block >= blocks.size() || relative > allocation.count ||
        static_cast<uint64_t>(len) > allocation.count - relative ||
        allocation.offset >= arena_location_block_values ||
        relative > arena_location_block_values - allocation.offset ||
        static_cast<uint64_t>(len) >
            arena_location_block_values - allocation.offset - relative)
      return false;
    const uint64_t code = (static_cast<uint64_t>(allocation.block)
                           << arena_location_offset_bits) |
                          static_cast<uint64_t>(allocation.offset + relative);
    if (code >= static_cast<uint64_t>(arena_location_count)) return false;
    result =
        -static_cast<double>(arena_location_first + static_cast<int64_t>(code));
    return true;
  }

  double* value(double h) const noexcept {
    // location_handle() validates every coordinate before publication. Arena
    // blocks remain stable until all evaluation-local handles are discarded.
    const uint64_t code =
        static_cast<uint64_t>(-h - static_cast<double>(arena_location_first));
    const size_t block_index =
        static_cast<size_t>(code >> arena_location_offset_bits);
    const size_t block_offset = static_cast<size_t>(
        code & static_cast<uint64_t>(arena_location_block_values - 1));
    const Block& block = blocks[block_index];
    return block.data.get() + block_offset;
  }
};

struct DynamicAllocationProfile {
  struct Bucket {
    uint64_t ordinary_visits = 0;
    uint64_t active_visits = 0;
    uint64_t compact_visits = 0;
    uint64_t frame_free_compact_visits = 0;
    uint64_t invariant_reuses = 0;
    uint64_t inactive_control_calls = 0;
    uint64_t inactive_workspace_calls = 0;
    uint64_t direct_control_calls = 0;
    uint64_t inline_integer_calls = 0;
    uint64_t elided_arena_values = 0;
    uint64_t elided_refs = 0;
    uint64_t inactive_transient_values = 0;
    uint64_t inactive_output_values = 0;
    uint64_t inactive_output_refs = 0;
    uint64_t arena_location_refs = 0;
    uint64_t arena_values = 0;
    uint64_t input_handle_values = 0;
    uint64_t record_handle_values = 0;
    uint64_t compact_position_values = 0;
    uint64_t compact_old_value_values = 0;
    uint64_t compact_rhs_handle_values = 0;
    uint64_t output_values = 0;
    uint64_t scratch_values = 0;
    uint64_t padding_values = 0;
    uint64_t refs = 0;
    uint64_t active_refs = 0;
    uint64_t referenced_values = 0;
    uint64_t conceptual_adjoint_values = 0;
    uint64_t physical_adjoint_values = 0;
    uint64_t record_embedded_primal_values = 0;
    uint64_t record_embedded_adjoint_values = 0;
    uint64_t embedded_scalar_index_visits = 0;
    uint64_t empty_embedded_scalar_index_visits = 0;
    uint64_t records = 0;
  };

  std::array<Bucket, OP_COUNT_> buckets{};
  uint64_t initial_arena_values = 0;
  uint64_t initial_slot_refs = 0;
  uint64_t canonical_refs = 0;
  uint64_t import_refs = 0;
  bool overflow = false;

  void add(uint64_t& total, uint64_t value) noexcept {
    if (total > std::numeric_limits<uint64_t>::max() - value) {
      total = std::numeric_limits<uint64_t>::max();
      overflow = true;
    } else {
      total += value;
    }
  }

  uint64_t count(int64_t value) noexcept {
    if (value < 0) {
      overflow = true;
      return 0;
    }
    return static_cast<uint64_t>(value);
  }

  Bucket* bucket(const Op& op) noexcept {
    if (op.opcode >= OP_COUNT_) {
      overflow = true;
      return nullptr;
    }
    return &buckets[op.opcode];
  }

  void note_reuse(const Op& op, uint64_t Bucket::* field) noexcept {
    if (Bucket* b = bucket(op)) add(b->*field, 1);
  }

  void note_ordinary(const Op& op, const Node& node, int64_t retained,
                     bool active) noexcept {
    Bucket* b = bucket(op);
    if (!b) return;
    const uint64_t inputs = count(op.n_in);
    const uint64_t outputs = count(node.frame_size - 6 - node.kernel_scratch);
    const uint64_t scratch = count(node.kernel_scratch);
    const uint64_t frame = count(retained);
    uint64_t components = inputs;
    add(components, outputs);
    add(components, scratch);
    const uint64_t padding = frame >= components ? frame - components : 0;
    if (frame < components) overflow = true;
    const uint64_t refs = op.out2 >= 0 ? 2 : 1;

    add(b->ordinary_visits, 1);
    add(b->active_visits, active ? 1 : 0);
    add(b->arena_values, frame);
    add(b->input_handle_values, inputs);
    add(b->output_values, outputs);
    add(b->scratch_values, scratch);
    add(b->padding_values, padding);
    add(b->refs, refs);
    add(b->active_refs, active ? refs : 0);
    add(b->referenced_values, outputs);
    add(b->conceptual_adjoint_values, active ? outputs : 0);
    add(b->physical_adjoint_values, active ? outputs : 0);
    add(b->records, active ? 1 : 0);
    if (!active) {
      uint64_t transient = inputs;
      add(transient, scratch);
      add(transient, padding);
      add(b->inactive_transient_values, transient);
      add(b->inactive_output_values, outputs);
      add(b->inactive_output_refs, refs);
    }
  }

  void note_inactive_split(const Op& op, const Node& node,
                           int64_t retained_outputs,
                           uint64_t arena_locations) noexcept {
    Bucket* b = bucket(op);
    if (!b) return;
    const uint64_t inputs = count(op.n_in);
    const uint64_t outputs = count(node.frame_size - 6 - node.kernel_scratch);
    const uint64_t scratch = count(node.kernel_scratch);
    const uint64_t old_frame = count(std::max<int64_t>(
        1, node.frame_size - (6 - static_cast<int64_t>(op.n_in))));
    uint64_t old_components = inputs;
    add(old_components, outputs);
    add(old_components, scratch);
    const uint64_t old_padding =
        old_frame >= old_components ? old_frame - old_components : 0;
    if (old_frame < old_components) overflow = true;
    const uint64_t new_frame = count(retained_outputs);
    const uint64_t new_padding = new_frame >= outputs ? new_frame - outputs : 0;
    if (new_frame < outputs) overflow = true;
    const uint64_t refs = op.out2 >= 0 ? 2 : 1;
    if (arena_locations > refs) {
      arena_locations = refs;
      overflow = true;
    }
    uint64_t transient = inputs;
    add(transient, scratch);
    add(transient, old_padding);

    add(b->ordinary_visits, 1);
    add(b->arena_values, new_frame);
    add(b->output_values, outputs);
    add(b->padding_values, new_padding);
    add(b->refs, refs - arena_locations);
    add(b->elided_refs, arena_locations);
    add(b->arena_location_refs, arena_locations);
    add(b->referenced_values, outputs);
    add(b->inactive_transient_values, transient);
    add(b->inactive_output_values, outputs);
    add(b->inactive_output_refs, refs);
  }

  void note_inactive_workspace(const Op& op, const Node& node) noexcept {
    Bucket* b = bucket(op);
    if (!b) return;
    const uint64_t inputs = count(op.n_in);
    const uint64_t outputs = count(node.frame_size - 6 - node.kernel_scratch);
    const uint64_t scratch = count(node.kernel_scratch);
    const uint64_t old_frame = count(std::max<int64_t>(
        1, node.frame_size - (6 - static_cast<int64_t>(op.n_in))));
    uint64_t old_components = inputs;
    add(old_components, outputs);
    add(old_components, scratch);
    const uint64_t old_padding =
        old_frame >= old_components ? old_frame - old_components : 0;
    if (old_frame < old_components) overflow = true;
    uint64_t transient = inputs;
    add(transient, scratch);
    add(transient, old_padding);
    const uint64_t refs = op.out2 >= 0 ? 2 : 1;
    add(b->ordinary_visits, 1);
    add(b->inactive_workspace_calls, 1);
    add(b->elided_arena_values, outputs);
    add(b->elided_refs, refs);
    add(b->inactive_output_values, outputs);
    add(b->inactive_output_refs, refs);
    add(b->inactive_transient_values, transient);
    add(b->referenced_values, outputs);
  }

  void note_compact(const Op& op, int64_t retained, int64_t output_len,
                    bool active, bool shared_adjoint, bool frame_free,
                    bool reused_output_ref) noexcept {
    Bucket* b = bucket(op);
    if (!b) return;
    const uint64_t frame = count(retained);
    const uint64_t extent = count(output_len);
    add(b->compact_visits, 1);
    add(b->frame_free_compact_visits, frame_free ? 1 : 0);
    add(b->active_visits, active ? 1 : 0);
    add(b->arena_values, frame);
    add(b->input_handle_values, frame);
    add(b->record_handle_values, frame_free && active ? 1 : 0);
    add(b->refs, reused_output_ref ? 0 : 1);
    add(b->active_refs, active && !reused_output_ref ? 1 : 0);
    add(b->elided_refs, reused_output_ref ? 1 : 0);
    add(b->referenced_values, extent);
    add(b->conceptual_adjoint_values, active ? extent : 0);
    add(b->physical_adjoint_values, active && !shared_adjoint ? extent : 0);
    add(b->records, 1);
  }

  void note_delta_compact(const Op& op, int64_t selected, int64_t output_len,
                          bool active) noexcept {
    Bucket* b = bucket(op);
    if (!b) return;
    const uint64_t count = this->count(selected);
    const uint64_t extent = this->count(output_len);
    uint64_t retained = count;
    add(retained, count);
    add(retained, active ? 1 : 0);
    add(b->compact_visits, 1);
    add(b->frame_free_compact_visits, 1);
    add(b->active_visits, active ? 1 : 0);
    add(b->arena_values, retained);
    add(b->compact_position_values, count);
    add(b->compact_old_value_values, count);
    add(b->compact_rhs_handle_values, active ? 1 : 0);
    add(b->refs, 1);
    add(b->active_refs, active ? 1 : 0);
    add(b->referenced_values, extent);
    add(b->conceptual_adjoint_values, active ? extent : 0);
    add(b->records, 1);
  }

  void note_compact_index(const Op& op, bool embedded,
                          bool empty = false) noexcept {
    Bucket* b = bucket(op);
    if (!b) return;
    add(b->compact_visits, 1);
    add(b->frame_free_compact_visits, 1);
    add(b->active_visits, 1);
    add(b->record_handle_values, 1);
    add(b->output_values, 1);
    add(b->referenced_values, 1);
    add(b->conceptual_adjoint_values, 1);
    add(b->physical_adjoint_values, 1);
    add(b->records, 1);
    if (embedded) {
      add(b->elided_arena_values, 1);
      add(b->elided_refs, 1);
      add(b->record_embedded_primal_values, 1);
      add(b->record_embedded_adjoint_values, 1);
      add(b->embedded_scalar_index_visits, 1);
      add(b->empty_embedded_scalar_index_visits, empty ? 1 : 0);
    } else {
      add(b->arena_values, 1);
      add(b->refs, 1);
      add(b->active_refs, 1);
    }
  }

  void note_inline_integer(const Op& op, int64_t retained) noexcept {
    Bucket* b = bucket(op);
    if (!b) return;
    add(b->inline_integer_calls, 1);
    add(b->elided_arena_values, count(retained));
    add(b->elided_refs, 1);
  }
};

struct DynamicLoopState final : KernelState {
  static constexpr uint32_t ordinary_record =
      std::numeric_limits<uint32_t>::max();
  static constexpr uint32_t retained_scalar_record = ordinary_record - 1;
  static constexpr uint32_t delta_record = ordinary_record - 2;
  static constexpr uint32_t scalar_index_record = ordinary_record - 3;
  static constexpr uint32_t scalar_index_empty_record = ordinary_record - 4;
  static constexpr uint32_t max_frame_free_position = ordinary_record - 5;

  struct Ref {
    double* value = nullptr;
    // Nonnegative values are internal adjoint offsets, -1 marks an inactive
    // internal value, and values <= -2 encode an ordinal in the immutable
    // StructuredLoop::imports table. The expected length comes from the graph
    // slot at every use and need not be repeated for every reached value.
    int64_t adjoint_or_import = -1;
  };
  struct Record {
    // Ordinary and retained compact records store their input frame. A
    // frame-free active compact record stores only its RHS handle; an inactive
    // frame-free compact record leaves the pointer null.
    union {
      double* frame = nullptr;
      double rhs_handle;
      double base_handle;
    };
    double out = -1;
    // Ordinary calls store the optional second-output handle. Scalar compact
    // updates store the overwritten value. Ref-backed compact scalar reads
    // store their exact reached position, or -1 for an empty selection;
    // record-resident reads use this cell as their local adjoint. Delta
    // compact updates store their exact reached selection count.
    double out2 = -1;
    uint32_t site = ~uint32_t{0};
    // Reserved tags identify ordinary, retained scalar update, ordered-delta,
    // and scalar-read records. Every lower code is an exact frame-free scalar
    // update position.
    uint32_t code = ordinary_record;
  };
  DynamicArena arena;
  std::vector<double> adjoints;
  std::vector<Ref> refs;
  std::vector<double> bindings;
  // One current evaluation-local anchor per admitted binding cell. A whole
  // assignment/reset changes the binding handle and therefore fails the
  // pointer identity check until the next ordinary copying update anchors it.
  std::vector<double*> compact_primal_by_cell;
  std::vector<double> target_refs;
  std::vector<double> target_work;
  std::vector<Record> records;
  // H2B may assign multiple conceptual compact-update Refs to one physical
  // internal-adjoint range. Reverse uses one fully overwritten temporary
  // output range to preserve the ordinary callback's distinct descriptors.
  std::unique_ptr<double[]> compact_adjoint_work;
  int64_t compact_adjoint_work_size = 0;
  // Reused only by synchronous inactive forward callbacks. Its maximum size
  // is immutable body metadata, so no per-visit scratch enters the tape.
  std::unique_ptr<double[]> inactive_scratch;
  int64_t inactive_scratch_size = 0;
  double inactive_empty_value = 0.0;
  // Pointer-free call-site templates. They retain only immutable graph
  // structure between calls; context() refreshes every evaluation-local
  // pointer and adjoint before each reached forward or reverse callback.
  std::vector<KernelCtx> context_templates;
  std::vector<const Node*> record_nodes;
  std::unique_ptr<DirectControlPlan> direct_control_plan;
  std::unique_ptr<LoopInvariantPlan> loop_invariant_plan;
  std::unique_ptr<InactiveControlPlan> inactive_control_plan;
  std::unique_ptr<InactiveWorkspacePlan> inactive_workspace_plan;
  std::unique_ptr<DynamicAllocationProfile> allocation_profile;
  std::vector<double> inactive_control_values;
  uint64_t inactive_workspace_allocated_values = 0;
  int64_t conceptual_adjoint_size = 0;
  int64_t adjoint_size = 0;
  uint64_t iterator_values = 0;
  bool loop_invariant_reuse = false;
  bool inactive_control_reuse = false;
  bool inactive_workspace_reuse = false;
  bool record_scalar_reuse = false;
  bool shared_update_ref_reuse = false;
  bool memory_profile = false;
  bool cached_context_in_use = false;
  // A dynamic tape belongs to exactly one completed forward sweep.  Forward
  // invalidates the previous tape before doing any validation or allocation;
  // backward consumes the published tape before it can mutate adjoints.
  bool reverse_ready = false;

  explicit DynamicLoopState(const StructuredLoop& p) {
    context_templates.reserve(p.body.ops.size());
    for (const Op& op : p.body.ops) {
      const Kernel* kernel = find_kernel(op.opcode);
      if (!kernel)
        throw std::invalid_argument(
            "unregistered dynamic structured body kernel");
      // A nested/stateful kernel needs a distinct per-call state lifetime.
      // Dynamic structured execution has no such ownership contract yet, so
      // caching (and execution) must fail closed rather than share null state.
      if (kernel->make_state)
        throw std::invalid_argument(
            "stateful dynamic structured body kernel is unsupported");
      if (op.n_in < 0 || op.n_in > 6 || op.out < 0 ||
          static_cast<size_t>(op.out) >= p.body.slots.size() ||
          (op.out2 >= 0 && static_cast<size_t>(op.out2) >= p.body.slots.size()))
        throw std::invalid_argument(
            "dynamic structured context template is invalid");
      KernelCtx c{};
      c.n_in = op.n_in;
      c.variant = op.variant;
      c.idata = op.idata;
      c.n_idata = op.n_idata;
      c.udata = op.udata;
      for (int k = 0; k < op.n_in; ++k) {
        if (op.in[k] < 0 ||
            static_cast<size_t>(op.in[k]) >= p.body.slots.size())
          throw std::invalid_argument(
              "dynamic structured context input is invalid");
        const int64_t len = p.body.slots[op.in[k]].len;
        c.in[k] = {nullptr, len};
        c.in_adj[k] = {nullptr, len};
      }
      c.out = {nullptr, p.body.slots[op.out].len};
      c.out_adj_vec = {nullptr, c.out.len};
      if (op.out2 >= 0) c.out2 = {nullptr, p.body.slots[op.out2].len};
      context_templates.push_back(c);
    }
    record_nodes.assign(p.record_node_count, nullptr);
    const auto visit_workspace = [&](const auto& self, const Node& n) -> void {
      if (n.kind == Node::KernelCall) {
        if (n.op < 0 || static_cast<size_t>(n.op) >= p.body.ops.size())
          throw std::invalid_argument("dynamic structured body op is invalid");
        if (n.record_site >= record_nodes.size() ||
            record_nodes[n.record_site] != nullptr)
          throw std::invalid_argument(
              "dynamic structured record site is invalid");
        record_nodes[n.record_site] = &n;
        inactive_scratch_size =
            std::max(inactive_scratch_size, n.kernel_scratch);
        if (n.compact_update_cell >= 0) {
          const Op& op = p.body.ops[static_cast<size_t>(n.op)];
          const int64_t len = p.body.slots[static_cast<size_t>(op.out)].len;
          if (len <= 0)
            throw std::invalid_argument(
                "compact dynamic structured output extent is invalid");
          const auto* spec = static_cast<const DynamicIndexSpec*>(op.udata);
          if (spec && scalar_compact_update_spec(*spec))
            compact_adjoint_work_size =
                std::max(compact_adjoint_work_size, len);
        }
      }
      for (const auto& child : n.children) self(self, child);
    };
    visit_workspace(visit_workspace, p.root);
    if (std::any_of(record_nodes.begin(), record_nodes.end(),
                    [](const Node* node) { return node == nullptr; }))
      throw std::invalid_argument("dynamic structured record site is missing");
    if (compact_adjoint_work_size < 0 ||
        static_cast<uint64_t>(compact_adjoint_work_size) >
            std::numeric_limits<size_t>::max())
      throw std::length_error("compact structured adjoint work overflow");
    if (compact_adjoint_work_size > 0)
      compact_adjoint_work.reset(
          new double[static_cast<size_t>(compact_adjoint_work_size)]);
    try {
      if (inactive_scratch_size < 0 ||
          static_cast<uint64_t>(inactive_scratch_size) >
              std::numeric_limits<size_t>::max())
        throw std::length_error("inactive structured scratch overflow");
      if (inactive_scratch_size > 0)
        inactive_scratch.reset(
            new double[static_cast<size_t>(inactive_scratch_size)]);
    } catch (...) {
      // This workspace is optional. Preserve the ordinary retained-frame path
      // if validation or eager allocation cannot be satisfied at bind time.
      inactive_scratch.reset();
      inactive_scratch_size = 0;
    }
    // This body-sized proof is built once with the bound Executor and never
    // rebuilt from reached iterations or observed parameter values.
    try {
      direct_control_plan = std::make_unique<DirectControlPlan>(p);
    } catch (...) {
      // Direct-control reuse is an optional optimization. Classifier
      // allocation or proof failure must not reduce ordinary model coverage.
      direct_control_plan.reset();
    }
    try {
      loop_invariant_plan = std::make_unique<LoopInvariantPlan>(p);
      if (loop_invariant_plan->sites.empty()) loop_invariant_plan.reset();
    } catch (...) {
      // Invariant reuse is optional. A classifier allocation or proof failure
      // leaves the ordinary retained executor authoritative.
      loop_invariant_plan.reset();
    }
    try {
      inactive_control_plan = std::make_unique<InactiveControlPlan>(p);
      if (inactive_control_plan->cones.empty()) {
        inactive_control_plan.reset();
      } else {
        if (inactive_control_plan->value_count >
            std::numeric_limits<size_t>::max())
          throw std::length_error("inactive-control value storage overflow");
        inactive_control_values.resize(
            static_cast<size_t>(inactive_control_plan->value_count));
      }
    } catch (...) {
      // Control-history elision is optional. Setup failure preserves the
      // ordinary per-call history path and model coverage.
      inactive_control_plan.reset();
      std::vector<double>{}.swap(inactive_control_values);
    }
    try {
      inactive_workspace_plan = std::make_unique<InactiveWorkspacePlan>(
          p, loop_invariant_plan.get(), inactive_control_plan.get());
      if (inactive_workspace_plan->sites.empty())
        inactive_workspace_plan.reset();
    } catch (...) {
      // Forward-only workspace reuse is optional.  Allocation or proof
      // failure leaves every call on the persistent dynamic arena path.
      inactive_workspace_plan.reset();
    }
  }

  void release_tape() noexcept {
    reverse_ready = false;
    arena.clear();
    std::vector<double>{}.swap(adjoints);
    std::vector<Ref>{}.swap(refs);
    std::vector<double>{}.swap(bindings);
    std::vector<double*>{}.swap(compact_primal_by_cell);
    std::vector<double>{}.swap(target_refs);
    std::vector<double>{}.swap(target_work);
    std::vector<Record>{}.swap(records);
    conceptual_adjoint_size = 0;
    adjoint_size = 0;
    iterator_values = 0;
    cached_context_in_use = false;
    memory_profile = false;
    allocation_profile.reset();
  }
};
static_assert(sizeof(DynamicLoopState::Ref) == 16,
              "dynamic structured references must stay compact");
static_assert(sizeof(DynamicLoopState::Record) == 32,
              "dynamic structured reverse records must stay compact");
static_assert(std::is_trivially_copyable_v<DynamicLoopState::Record>,
              "dynamic structured reverse records must copy by value");

uint64_t profile_bytes(uint64_t count, uint64_t width,
                       bool& overflow) noexcept {
  if (width && count > std::numeric_limits<uint64_t>::max() / width) {
    overflow = true;
    return std::numeric_limits<uint64_t>::max();
  }
  return count * width;
}

uint64_t profile_add(uint64_t first, uint64_t second, bool& overflow) noexcept {
  if (first > std::numeric_limits<uint64_t>::max() - second) {
    overflow = true;
    return std::numeric_limits<uint64_t>::max();
  }
  return first + second;
}

void report_memory_profile(const StructuredLoop& p,
                           const DynamicLoopState& state) noexcept {
  const int saved_errno = errno;
  bool overflow = false;
  const uint64_t arena_used =
      profile_bytes(state.arena.used_values, sizeof(double), overflow);
  const uint64_t arena_capacity =
      profile_bytes(state.arena.capacity_values, sizeof(double), overflow);
  const uint64_t ref_used =
      profile_bytes(state.refs.size(), sizeof(DynamicLoopState::Ref), overflow);
  const uint64_t ref_capacity = profile_bytes(
      state.refs.capacity(), sizeof(DynamicLoopState::Ref), overflow);
  const uint64_t record_used = profile_bytes(
      state.records.size(), sizeof(DynamicLoopState::Record), overflow);
  const uint64_t record_capacity = profile_bytes(
      state.records.capacity(), sizeof(DynamicLoopState::Record), overflow);
  const uint64_t planned_adjoint = profile_bytes(
      static_cast<uint64_t>(state.adjoint_size), sizeof(double), overflow);
  const uint64_t inactive_scratch =
      profile_bytes(static_cast<uint64_t>(state.inactive_scratch_size),
                    sizeof(double), overflow);
  const uint64_t inactive_workspace = profile_bytes(
      state.inactive_workspace_allocated_values, sizeof(double), overflow);
  const uint64_t iterator_value_bytes =
      profile_bytes(state.iterator_values, sizeof(double), overflow);
  const uint64_t iterator_ref_bytes = profile_bytes(
      state.iterator_values, sizeof(DynamicLoopState::Ref), overflow);
  const uint64_t iterator_total_bytes =
      profile_add(iterator_value_bytes, iterator_ref_bytes, overflow);
  std::fprintf(
      stderr,
      "stanli_structured_memory phase=forward region=%p nodes=%zu "
      "body_kernels=%zu accounting_overflow=%d "
      "elided_iterator_instances=%llu "
      "elided_iterator_value_bytes=%llu "
      "elided_iterator_ref_bytes=%llu "
      "elided_iterator_total_bytes=%llu "
      "arena_used_bytes=%llu arena_capacity_bytes=%llu "
      "ref_count=%zu ref_used_bytes=%llu ref_capacity_bytes=%llu "
      "record_count=%zu record_used_bytes=%llu record_capacity_bytes=%llu "
      "undo_count=0 undo_used_bytes=0 undo_capacity_bytes=0 "
      "planned_adjoint_bytes=%llu inactive_scratch_bytes=%llu "
      "inactive_workspace_bytes=%llu\n",
      static_cast<const void*>(&p), p.node_count, p.body.ops.size(),
      overflow ? 1 : 0, static_cast<unsigned long long>(state.iterator_values),
      static_cast<unsigned long long>(iterator_value_bytes),
      static_cast<unsigned long long>(iterator_ref_bytes),
      static_cast<unsigned long long>(iterator_total_bytes),
      static_cast<unsigned long long>(arena_used),
      static_cast<unsigned long long>(arena_capacity), state.refs.size(),
      static_cast<unsigned long long>(ref_used),
      static_cast<unsigned long long>(ref_capacity), state.records.size(),
      static_cast<unsigned long long>(record_used),
      static_cast<unsigned long long>(record_capacity),
      static_cast<unsigned long long>(planned_adjoint),
      static_cast<unsigned long long>(inactive_scratch),
      static_cast<unsigned long long>(inactive_workspace));

  const auto* allocation = state.allocation_profile.get();
  if (!allocation) {
    errno = saved_errno;
    return;
  }
  bool allocation_overflow = allocation->overflow;
  uint64_t kernel_arena_values = 0;
  uint64_t input_handle_values = 0;
  uint64_t record_handle_values = 0;
  uint64_t compact_position_values = 0;
  uint64_t compact_old_value_values = 0;
  uint64_t compact_rhs_handle_values = 0;
  uint64_t output_values = 0;
  uint64_t scratch_values = 0;
  uint64_t padding_values = 0;
  uint64_t kernel_refs = 0;
  uint64_t active_refs = 0;
  uint64_t referenced_values = 0;
  uint64_t conceptual_adjoint_values = 0;
  uint64_t physical_adjoint_values = 0;
  uint64_t record_embedded_primal_values = 0;
  uint64_t record_embedded_adjoint_values = 0;
  uint64_t embedded_scalar_index_visits = 0;
  uint64_t empty_embedded_scalar_index_visits = 0;
  uint64_t profiled_records = 0;
  uint64_t frame_free_compact_visits = 0;
  uint64_t inline_integer_calls = 0;
  uint64_t elided_arena_values = 0;
  uint64_t elided_refs = 0;
  uint64_t inactive_transient_values = 0;
  uint64_t inactive_output_values = 0;
  uint64_t inactive_output_refs = 0;
  uint64_t arena_location_refs = 0;
  uint64_t inactive_workspace_calls = 0;
  uint64_t inactive_workspace_refs = 0;
  if (state.inactive_workspace_plan)
    for (const auto& site : state.inactive_workspace_plan->sites)
      if (site.canonical_ready) ++inactive_workspace_refs;
  for (const auto& bucket : allocation->buckets) {
    kernel_arena_values = profile_add(kernel_arena_values, bucket.arena_values,
                                      allocation_overflow);
    input_handle_values = profile_add(
        input_handle_values, bucket.input_handle_values, allocation_overflow);
    record_handle_values = profile_add(
        record_handle_values, bucket.record_handle_values, allocation_overflow);
    compact_position_values =
        profile_add(compact_position_values, bucket.compact_position_values,
                    allocation_overflow);
    compact_old_value_values =
        profile_add(compact_old_value_values, bucket.compact_old_value_values,
                    allocation_overflow);
    compact_rhs_handle_values =
        profile_add(compact_rhs_handle_values, bucket.compact_rhs_handle_values,
                    allocation_overflow);
    output_values =
        profile_add(output_values, bucket.output_values, allocation_overflow);
    scratch_values =
        profile_add(scratch_values, bucket.scratch_values, allocation_overflow);
    padding_values =
        profile_add(padding_values, bucket.padding_values, allocation_overflow);
    kernel_refs = profile_add(kernel_refs, bucket.refs, allocation_overflow);
    active_refs =
        profile_add(active_refs, bucket.active_refs, allocation_overflow);
    referenced_values = profile_add(referenced_values, bucket.referenced_values,
                                    allocation_overflow);
    conceptual_adjoint_values =
        profile_add(conceptual_adjoint_values, bucket.conceptual_adjoint_values,
                    allocation_overflow);
    physical_adjoint_values =
        profile_add(physical_adjoint_values, bucket.physical_adjoint_values,
                    allocation_overflow);
    record_embedded_primal_values =
        profile_add(record_embedded_primal_values,
                    bucket.record_embedded_primal_values, allocation_overflow);
    record_embedded_adjoint_values =
        profile_add(record_embedded_adjoint_values,
                    bucket.record_embedded_adjoint_values, allocation_overflow);
    embedded_scalar_index_visits =
        profile_add(embedded_scalar_index_visits,
                    bucket.embedded_scalar_index_visits, allocation_overflow);
    empty_embedded_scalar_index_visits = profile_add(
        empty_embedded_scalar_index_visits,
        bucket.empty_embedded_scalar_index_visits, allocation_overflow);
    profiled_records =
        profile_add(profiled_records, bucket.records, allocation_overflow);
    frame_free_compact_visits =
        profile_add(frame_free_compact_visits, bucket.frame_free_compact_visits,
                    allocation_overflow);
    inline_integer_calls = profile_add(
        inline_integer_calls, bucket.inline_integer_calls, allocation_overflow);
    elided_arena_values = profile_add(
        elided_arena_values, bucket.elided_arena_values, allocation_overflow);
    elided_refs =
        profile_add(elided_refs, bucket.elided_refs, allocation_overflow);
    inactive_transient_values =
        profile_add(inactive_transient_values,
                    bucket.inactive_transient_values, allocation_overflow);
    inactive_output_values =
        profile_add(inactive_output_values, bucket.inactive_output_values,
                    allocation_overflow);
    inactive_output_refs =
        profile_add(inactive_output_refs, bucket.inactive_output_refs,
                    allocation_overflow);
    arena_location_refs = profile_add(
        arena_location_refs, bucket.arena_location_refs, allocation_overflow);
    inactive_workspace_calls =
        profile_add(inactive_workspace_calls, bucket.inactive_workspace_calls,
                    allocation_overflow);
  }
  uint64_t profiled_arena_values =
      profile_add(allocation->initial_arena_values, kernel_arena_values,
                  allocation_overflow);
  uint64_t setup_refs =
      profile_add(allocation->initial_slot_refs, allocation->canonical_refs,
                  allocation_overflow);
  setup_refs =
      profile_add(setup_refs, allocation->import_refs, allocation_overflow);
  setup_refs =
      profile_add(setup_refs, inactive_workspace_refs, allocation_overflow);
  const uint64_t profiled_refs =
      profile_add(setup_refs, kernel_refs, allocation_overflow);
  std::fprintf(
      stderr,
      "stanli_structured_alloc_summary region=%p accounting_overflow=%d "
      "initial_arena_values=%llu kernel_arena_values=%llu "
      "profiled_arena_values=%llu actual_arena_values=%llu arena_match=%d "
      "input_handle_values=%llu record_handle_values=%llu "
      "compact_position_values=%llu compact_old_value_values=%llu "
      "compact_rhs_handle_values=%llu "
      "output_values=%llu scratch_values=%llu "
      "padding_values=%llu initial_slot_refs=%llu canonical_refs=%llu "
      "import_refs=%llu inactive_workspace_refs=%llu kernel_refs=%llu "
      "profiled_refs=%llu "
      "actual_refs=%zu refs_match=%d active_refs=%llu "
      "referenced_values=%llu conceptual_adjoint_values=%llu "
      "physical_adjoint_values=%llu "
      "record_embedded_primal_values=%llu "
      "record_embedded_adjoint_values=%llu "
      "embedded_scalar_index_visits=%llu "
      "empty_embedded_scalar_index_visits=%llu profiled_records=%llu "
      "actual_records=%zu records_match=%d inline_integer_calls=%llu "
      "frame_free_compact_visits=%llu "
      "elided_arena_values=%llu elided_refs=%llu "
      "inactive_transient_values=%llu inactive_output_values=%llu "
      "inactive_output_refs=%llu arena_location_refs=%llu "
      "inactive_workspace_calls=%llu inactive_workspace_values=%llu\n",
      static_cast<const void*>(&p), allocation_overflow ? 1 : 0,
      static_cast<unsigned long long>(allocation->initial_arena_values),
      static_cast<unsigned long long>(kernel_arena_values),
      static_cast<unsigned long long>(profiled_arena_values),
      static_cast<unsigned long long>(state.arena.used_values),
      profiled_arena_values == state.arena.used_values ? 1 : 0,
      static_cast<unsigned long long>(input_handle_values),
      static_cast<unsigned long long>(record_handle_values),
      static_cast<unsigned long long>(compact_position_values),
      static_cast<unsigned long long>(compact_old_value_values),
      static_cast<unsigned long long>(compact_rhs_handle_values),
      static_cast<unsigned long long>(output_values),
      static_cast<unsigned long long>(scratch_values),
      static_cast<unsigned long long>(padding_values),
      static_cast<unsigned long long>(allocation->initial_slot_refs),
      static_cast<unsigned long long>(allocation->canonical_refs),
      static_cast<unsigned long long>(allocation->import_refs),
      static_cast<unsigned long long>(inactive_workspace_refs),
      static_cast<unsigned long long>(kernel_refs),
      static_cast<unsigned long long>(profiled_refs), state.refs.size(),
      profiled_refs == state.refs.size() ? 1 : 0,
      static_cast<unsigned long long>(active_refs),
      static_cast<unsigned long long>(referenced_values),
      static_cast<unsigned long long>(conceptual_adjoint_values),
      static_cast<unsigned long long>(physical_adjoint_values),
      static_cast<unsigned long long>(record_embedded_primal_values),
      static_cast<unsigned long long>(record_embedded_adjoint_values),
      static_cast<unsigned long long>(embedded_scalar_index_visits),
      static_cast<unsigned long long>(empty_embedded_scalar_index_visits),
      static_cast<unsigned long long>(profiled_records), state.records.size(),
      profiled_records == state.records.size() ? 1 : 0,
      static_cast<unsigned long long>(inline_integer_calls),
      static_cast<unsigned long long>(frame_free_compact_visits),
      static_cast<unsigned long long>(elided_arena_values),
      static_cast<unsigned long long>(elided_refs),
      static_cast<unsigned long long>(inactive_transient_values),
      static_cast<unsigned long long>(inactive_output_values),
      static_cast<unsigned long long>(inactive_output_refs),
      static_cast<unsigned long long>(arena_location_refs),
      static_cast<unsigned long long>(inactive_workspace_calls),
      static_cast<unsigned long long>(
          state.inactive_workspace_allocated_values));

  for (uint16_t opcode = 0; opcode < OP_COUNT_; ++opcode) {
    const auto& bucket = allocation->buckets[opcode];
    if (bucket.ordinary_visits == 0 && bucket.compact_visits == 0 &&
        bucket.invariant_reuses == 0 && bucket.inactive_control_calls == 0 &&
        bucket.inactive_workspace_calls == 0 &&
        bucket.direct_control_calls == 0 && bucket.inline_integer_calls == 0)
      continue;
    std::fprintf(
        stderr,
        "stanli_structured_alloc_opcode region=%p opcode=%u name=%s "
        "ordinary_visits=%llu active_visits=%llu compact_visits=%llu "
        "frame_free_compact_visits=%llu "
        "invariant_reuses=%llu inactive_control_calls=%llu "
        "inactive_workspace_calls=%llu "
        "direct_control_calls=%llu inline_integer_calls=%llu "
        "elided_arena_values=%llu elided_refs=%llu arena_values=%llu "
        "inactive_transient_values=%llu inactive_output_values=%llu "
        "inactive_output_refs=%llu "
        "arena_location_refs=%llu "
        "input_handle_values=%llu record_handle_values=%llu "
        "compact_position_values=%llu compact_old_value_values=%llu "
        "compact_rhs_handle_values=%llu "
        "output_values=%llu scratch_values=%llu "
        "padding_values=%llu refs=%llu active_refs=%llu "
        "referenced_values=%llu conceptual_adjoint_values=%llu "
        "physical_adjoint_values=%llu "
        "record_embedded_primal_values=%llu "
        "record_embedded_adjoint_values=%llu "
        "embedded_scalar_index_visits=%llu "
        "empty_embedded_scalar_index_visits=%llu records=%llu\n",
        static_cast<const void*>(&p), static_cast<unsigned>(opcode),
        opcode_name(opcode),
        static_cast<unsigned long long>(bucket.ordinary_visits),
        static_cast<unsigned long long>(bucket.active_visits),
        static_cast<unsigned long long>(bucket.compact_visits),
        static_cast<unsigned long long>(bucket.frame_free_compact_visits),
        static_cast<unsigned long long>(bucket.invariant_reuses),
        static_cast<unsigned long long>(bucket.inactive_control_calls),
        static_cast<unsigned long long>(bucket.inactive_workspace_calls),
        static_cast<unsigned long long>(bucket.direct_control_calls),
        static_cast<unsigned long long>(bucket.inline_integer_calls),
        static_cast<unsigned long long>(bucket.elided_arena_values),
        static_cast<unsigned long long>(bucket.elided_refs),
        static_cast<unsigned long long>(bucket.arena_values),
        static_cast<unsigned long long>(bucket.inactive_transient_values),
        static_cast<unsigned long long>(bucket.inactive_output_values),
        static_cast<unsigned long long>(bucket.inactive_output_refs),
        static_cast<unsigned long long>(bucket.arena_location_refs),
        static_cast<unsigned long long>(bucket.input_handle_values),
        static_cast<unsigned long long>(bucket.record_handle_values),
        static_cast<unsigned long long>(bucket.compact_position_values),
        static_cast<unsigned long long>(bucket.compact_old_value_values),
        static_cast<unsigned long long>(bucket.compact_rhs_handle_values),
        static_cast<unsigned long long>(bucket.output_values),
        static_cast<unsigned long long>(bucket.scratch_values),
        static_cast<unsigned long long>(bucket.padding_values),
        static_cast<unsigned long long>(bucket.refs),
        static_cast<unsigned long long>(bucket.active_refs),
        static_cast<unsigned long long>(bucket.referenced_values),
        static_cast<unsigned long long>(bucket.conceptual_adjoint_values),
        static_cast<unsigned long long>(bucket.physical_adjoint_values),
        static_cast<unsigned long long>(bucket.record_embedded_primal_values),
        static_cast<unsigned long long>(bucket.record_embedded_adjoint_values),
        static_cast<unsigned long long>(bucket.embedded_scalar_index_visits),
        static_cast<unsigned long long>(
            bucket.empty_embedded_scalar_index_visits),
        static_cast<unsigned long long>(bucket.records));
  }
  errno = saved_errno;
}

int64_t compact_scalar_update_position(const DynamicIndexSpec& p,
                                       const KernelCtx& c);
int64_t compact_ordered_update_positions(const DynamicIndexSpec& p,
                                         const KernelCtx& c, double* positions);

bool overlaps(Desc a, Desc b) {
  if (!a.data || !b.data || a.len <= 0 || b.len <= 0) return false;
  const std::less<double*> before;
  return before(a.data, b.data + b.len) && before(b.data, a.data + a.len);
}

DynamicLoopState& dynamic_state(KernelCtx& c) {
  if (!c.state) throw std::logic_error("dynamic structured loop has no state");
  return *static_cast<DynamicLoopState*>(c.state);
}

void compare_forward(KernelCtx& c);
void int_forward(KernelCtx& c);

struct DynamicExecution {
  const StructuredLoop& p;
  KernelCtx& outer;
  DynamicLoopState& state;
  double* initial_values = nullptr;

  explicit DynamicExecution(KernelCtx& c)
      : p(*static_cast<const StructuredLoop*>(c.udata)),
        outer(c),
        state(dynamic_state(c)) {}

  const Node& record_node(const DynamicLoopState::Record& record) const {
    if (record.site >= state.record_nodes.size() ||
        !state.record_nodes[record.site])
      throw std::logic_error("dynamic structured record site is invalid");
    return *state.record_nodes[record.site];
  }

  DynamicLoopState::Ref& ordinary_ref(double h) {
    const int64_t id = offset(h);
    if (id < 0 || static_cast<size_t>(id) >= state.refs.size())
      throw std::logic_error("dynamic structured value handle out of range");
    return state.refs[static_cast<size_t>(id)];
  }

  const Node& validate_record_scalar(
      const DynamicLoopState::Record& record) const {
    const Node& node = record_node(record);
    if (node.op < 0 || static_cast<size_t>(node.op) >= p.body.ops.size())
      throw std::logic_error("record scalar index node is invalid");
    const Op& op = p.body.ops[static_cast<size_t>(node.op)];
    const auto* spec = static_cast<const DynamicIndexSpec*>(op.udata);
    IndexInputLayout layout;
    if (!spec || !index_input_layout(*spec, false, layout) ||
        op.n_in != layout.expected || node.forward != index_forward ||
        node.backward != index_backward || op.opcode != OP_INDEX_DYNAMIC ||
        op.out2 >= 0 || spec->selected_size != 1 ||
        p.body.slots[static_cast<size_t>(op.out)].len != 1 ||
        node.kernel_scratch != 0)
      throw std::logic_error("record scalar index record is invalid");
    const int64_t base_len = p.body.slots[static_cast<size_t>(op.in[0])].len;
    const bool empty =
        record.code == DynamicLoopState::scalar_index_empty_record;
    if (base_len < 0 ||
        (!empty && (record.code > DynamicLoopState::max_frame_free_position ||
                    static_cast<uint64_t>(record.code) >=
                        static_cast<uint64_t>(base_len))))
      throw std::logic_error("record scalar index record is invalid");
    return node;
  }

  size_t record_scalar_id(double h) const {
    const uint64_t id =
        static_cast<uint64_t>(h - static_cast<double>(record_scalar_first));
    if (id >= state.records.size())
      throw std::logic_error(
          "dynamic structured record-scalar handle out of range");
    return static_cast<size_t>(id);
  }

  DynamicLoopState::Record& record_scalar(double h) {
    return state.records[record_scalar_id(h)];
  }

  DynamicLoopState::Ref& ref(double h) {
    if (is_inline_int(h) || is_arena_location(h) || is_record_scalar(h))
      throw std::logic_error(
          "dynamic structured immediate handle used as a reference");
    return ordinary_ref(h);
  }

  static bool is_import_ref(const DynamicLoopState::Ref& ref) noexcept {
    return ref.adjoint_or_import < -1;
  }

  const StructuredLoop::Import& import_ref(
      const DynamicLoopState::Ref& ref) const {
    if (!is_import_ref(ref))
      throw std::logic_error(
          "dynamic structured value is not an import reference");
    const uint64_t ordinal =
        static_cast<uint64_t>(-(ref.adjoint_or_import + 2));
    if (ordinal >= p.imports.size())
      throw std::logic_error(
          "dynamic structured import reference out of range");
    return p.imports[static_cast<size_t>(ordinal)];
  }

  double append_ref(DynamicLoopState::Ref ref, bool active) {
    if (state.refs.size() >= static_cast<size_t>(ordinary_ref_limit))
      throw std::length_error("dynamic structured reference overflow");
    const int64_t id = static_cast<int64_t>(state.refs.size());
    state.refs.push_back(ref);
    return handle(id, active);
  }

  double make_ref(double* value, int64_t len, bool active,
                  int64_t shared_adjoint_offset = -1) {
    if (len < 0)
      throw std::logic_error(
          "dynamic structured reference has negative length");
    DynamicLoopState::Ref r;
    r.value = value;
    if (active) {
      state.conceptual_adjoint_size = add(state.conceptual_adjoint_size, len);
      if (shared_adjoint_offset >= 0) {
        if (shared_adjoint_offset > state.adjoint_size ||
            len > state.adjoint_size - shared_adjoint_offset)
          throw std::logic_error(
              "shared structured adjoint range is out of bounds");
        r.adjoint_or_import = shared_adjoint_offset;
      } else {
        r.adjoint_or_import = state.adjoint_size;
        state.adjoint_size = add(state.adjoint_size, len);
      }
    } else if (shared_adjoint_offset >= 0) {
      throw std::logic_error(
          "inactive structured reference cannot share an adjoint");
    }
    return append_ref(r, active);
  }

  double make_compact_update_ref(double base_handle, double* value,
                                 int64_t len, bool active,
                                 int64_t shared_adjoint_offset,
                                 bool frame_free, bool& reused) {
    reused = false;
    if (!state.shared_update_ref_reuse || !active || !frame_free ||
        shared_adjoint_offset < 0 || base_handle < 0 ||
        is_record_scalar(base_handle))
      return make_ref(value, len, active, shared_adjoint_offset);

    const auto& base = ordinary_ref(base_handle);
    if (is_import_ref(base) || base.value != value ||
        base.adjoint_or_import != shared_adjoint_offset)
      return make_ref(value, len, active, shared_adjoint_offset);
    if (len < 0 || shared_adjoint_offset > state.adjoint_size ||
        len > state.adjoint_size - shared_adjoint_offset)
      return make_ref(value, len, active, shared_adjoint_offset);

    // This output is a new conceptual autodiff value, but the compact
    // update's ownership and shared-adjoint proofs make its physical Ref
    // identical to the base Ref. The reverse record still preserves every
    // reached update and observes the shared handle in the original order.
    state.conceptual_adjoint_size = add(state.conceptual_adjoint_size, len);
    reused = true;
    return base_handle;
  }

  double make_import_ref(double* value, int64_t len, bool active,
                         size_t import_ordinal) {
    if (len < 0)
      throw std::logic_error(
          "dynamic structured import reference has negative length");
    if (import_ordinal >= p.imports.size() ||
        import_ordinal >
            static_cast<uint64_t>(std::numeric_limits<int64_t>::max() - 2))
      throw std::length_error("dynamic structured import reference overflow");
    const auto& import = p.imports[import_ordinal];
    if (import.input < 0 || import.input >= outer.n_in || import.offset < 0 ||
        import.offset > outer.in[import.input].len ||
        len > outer.in[import.input].len - import.offset)
      throw std::logic_error(
          "dynamic structured import reference exceeds graph input");
    DynamicLoopState::Ref r;
    r.value = value;
    r.adjoint_or_import = -2 - static_cast<int64_t>(import_ordinal);
    return append_ref(r, active);
  }

  double scalar_value(double h) {
    if (is_inline_int(h)) return inline_int_value(h);
    if (is_arena_location(h)) return state.arena.value(h)[0];
    if (is_record_scalar(h)) return record_scalar(h).out;
    return ordinary_ref(h).value[0];
  }

  double scalar_value(int slot) {
    return scalar_value(state.bindings.at(static_cast<size_t>(slot)));
  }

  void copy_value(double h, int64_t len, double* output) {
    if (is_inline_int(h)) {
      if (len != 1)
        throw std::logic_error(
            "dynamic structured inline integer has nonscalar extent");
      output[0] = inline_int_value(h);
      return;
    }
    if (is_record_scalar(h)) {
      if (len != 1)
        throw std::logic_error(
            "dynamic structured record scalar has nonscalar extent");
      output[0] = record_scalar(h).out;
      return;
    }
    std::copy_n(
        is_arena_location(h) ? state.arena.value(h) : ordinary_ref(h).value,
        len, output);
  }

  bool value_overlaps(double h, int64_t len, Desc other) {
    if (is_inline_int(h)) {
      if (len != 1)
        throw std::logic_error(
            "dynamic structured inline integer has nonscalar extent");
      (void)inline_int_value(h);
      return false;
    }
    if (is_record_scalar(h)) {
      if (len != 1)
        throw std::logic_error(
            "dynamic structured record scalar has nonscalar extent");
      return overlaps({&record_scalar(h).out, 1}, other);
    }
    return overlaps(
        {is_arena_location(h) ? state.arena.value(h) : ordinary_ref(h).value,
         len},
        other);
  }

  void begin_loop_invariant_scope(const Node& n) noexcept {
    auto* plan = state.loop_invariant_plan.get();
    if (!plan || !state.loop_invariant_reuse) return;
    const int32_t id = plan->find_loop(n);
    if (id < 0 || static_cast<size_t>(id) >= plan->loops.size() ||
        plan->loops[static_cast<size_t>(id)].node != &n) {
      state.loop_invariant_reuse = false;
      return;
    }
    auto& generation = plan->loops[static_cast<size_t>(id)].generation;
    if (generation == std::numeric_limits<uint64_t>::max()) {
      state.loop_invariant_reuse = false;
      return;
    }
    ++generation;
  }

  LoopInvariantPlan::Site* loop_invariant_site(const Node& n) noexcept {
    auto* plan = state.loop_invariant_plan.get();
    if (!plan || !state.loop_invariant_reuse || n.op < 0 ||
        static_cast<size_t>(n.op) >= plan->site_by_op.size())
      return nullptr;
    const int32_t id = plan->site_by_op[static_cast<size_t>(n.op)];
    if (id < 0 || static_cast<size_t>(id) >= plan->sites.size()) return nullptr;
    auto& site = plan->sites[static_cast<size_t>(id)];
    if (site.node != &n || site.loop >= plan->loops.size()) {
      state.loop_invariant_reuse = false;
      return nullptr;
    }
    return &site;
  }

  bool reuse_loop_invariant(const Node& n) noexcept {
    auto* site = loop_invariant_site(n);
    if (!site) return false;
    auto* plan = state.loop_invariant_plan.get();
    auto& loop = plan->loops[site->loop];
    if (loop.generation == 0) {
      state.loop_invariant_reuse = false;
      return false;
    }
    const Op& op = p.body.ops[static_cast<size_t>(n.op)];
    bool active = n.backward != nullptr;
    bool any_active_input = false;
    for (int k = 0; k < op.n_in; ++k)
      any_active_input |= state.bindings[static_cast<size_t>(op.in[k])] >= 0;
    active &= any_active_input;
    if (active || site->cached_generation != loop.generation) return false;
    for (int k = 0; k < op.n_in; ++k)
      if (site->cached_inputs[static_cast<size_t>(k)] !=
          state.bindings[static_cast<size_t>(op.in[k])])
        return false;
    state.bindings[static_cast<size_t>(op.out)] = site->cached_out;
    if (op.out2 >= 0)
      state.bindings[static_cast<size_t>(op.out2)] = site->cached_out2;
    return true;
  }

  void publish_loop_invariant(const Node& n) noexcept {
    auto* site = loop_invariant_site(n);
    if (!site) return;
    auto* plan = state.loop_invariant_plan.get();
    auto& loop = plan->loops[site->loop];
    if (loop.generation == 0) {
      state.loop_invariant_reuse = false;
      return;
    }
    const Op& op = p.body.ops[static_cast<size_t>(n.op)];
    const double out = state.bindings[static_cast<size_t>(op.out)];
    const double out2 =
        op.out2 >= 0 ? state.bindings[static_cast<size_t>(op.out2)] : -1;
    if (out >= 0 || (op.out2 >= 0 && out2 >= 0)) return;
    for (int k = 0; k < op.n_in; ++k)
      site->cached_inputs[static_cast<size_t>(k)] =
          state.bindings[static_cast<size_t>(op.in[k])];
    site->cached_out = out;
    site->cached_out2 = out2;
    site->cached_generation = loop.generation;
  }

  InactiveControlPlan::Site* inactive_control_site(const Node& n) noexcept {
    auto* plan = state.inactive_control_plan.get();
    if (!plan || n.op < 0 ||
        static_cast<size_t>(n.op) >= plan->site_by_op.size())
      return nullptr;
    const int32_t id = plan->site_by_op[static_cast<size_t>(n.op)];
    if (id < 0 || static_cast<size_t>(id) >= plan->sites.size()) return nullptr;
    auto& site = plan->sites[static_cast<size_t>(id)];
    if (site.node != &n) {
      state.inactive_control_reuse = false;
      return nullptr;
    }
    return &site;
  }

  void begin_inactive_control_cone(InactiveControlPlan::Site* site) {
    if (!site || !site->first) return;
    auto* plan = state.inactive_control_plan.get();
    if (!plan || site->cone >= plan->cones.size()) {
      state.inactive_control_reuse = false;
      throw std::logic_error("inactive-control cone is out of range");
    }
    auto& cone = plan->cones[site->cone];
    if (cone.running) {
      state.inactive_control_reuse = false;
      throw std::logic_error("inactive-control cone reentry is unsupported");
    }
    cone.running = true;
    cone.decision = InactiveControlPlan::Cone::Ordinary;
    if (!state.inactive_control_reuse) return;
    for (int slot : cone.external_slots)
      if (state.bindings.at(static_cast<size_t>(slot)) >= 0) return;
    if (cone.value_offset > state.inactive_control_values.size() ||
        cone.value_count >
            state.inactive_control_values.size() - cone.value_offset) {
      state.inactive_control_reuse = false;
      throw std::logic_error("inactive-control cone value range is invalid");
    }
    const Desc output{state.inactive_control_values.data() + cone.value_offset,
                      static_cast<int64_t>(cone.value_count)};
    for (int slot : cone.external_slots)
      if (value_overlaps(state.bindings.at(static_cast<size_t>(slot)),
                         p.body.slots[slot].len, output))
        return;
    cone.decision = InactiveControlPlan::Cone::Direct;
  }

  void finish_inactive_control_site(InactiveControlPlan::Site* site) {
    if (!site) return;
    auto* plan = state.inactive_control_plan.get();
    if (!plan || site->cone >= plan->cones.size() ||
        !plan->cones[site->cone].running) {
      state.inactive_control_reuse = false;
      throw std::logic_error("inactive-control cone execution is incomplete");
    }
    if (site->last) plan->cones[site->cone].running = false;
  }

  void initialize_inactive_control_handles() noexcept {
    auto* plan = state.inactive_control_plan.get();
    if (!plan) return;
    try {
      for (auto& site : plan->sites) {
        const Op& op = p.body.ops.at(static_cast<size_t>(site.op));
        const Slot& output = p.body.slots.at(static_cast<size_t>(op.out));
        if (site.value_offset > state.inactive_control_values.size() ||
            output.len < 0 ||
            static_cast<uint64_t>(output.len) >
                state.inactive_control_values.size() - site.value_offset)
          throw std::logic_error(
              "inactive-control canonical result range is invalid");
        const double canonical =
            make_ref(state.inactive_control_values.data() + site.value_offset,
                     output.len, false);
        const auto& canonical_ref = ref(canonical);
        if (canonical >= 0 ||
            canonical_ref.value !=
                state.inactive_control_values.data() + site.value_offset ||
            canonical_ref.adjoint_or_import != -1)
          throw std::logic_error(
              "inactive-control canonical result handle is invalid");
        site.canonical_handle = canonical;
      }
    } catch (...) {
      // Canonical-handle setup is optional proof machinery. Any partial refs
      // are harmless evaluation-local entries; disable the plan and use the
      // ordinary path for this and later evaluations.
      state.inactive_control_plan.reset();
      state.inactive_control_reuse = false;
    }
  }

  InactiveWorkspacePlan::Site* inactive_workspace_site(const Node& n) noexcept {
    auto* plan = state.inactive_workspace_plan.get();
    if (!plan || !state.inactive_workspace_reuse || n.op < 0 ||
        static_cast<size_t>(n.op) >= plan->site_by_op.size())
      return nullptr;
    const int32_t id = plan->site_by_op[static_cast<size_t>(n.op)];
    if (id < 0 || static_cast<size_t>(id) >= plan->sites.size()) return nullptr;
    auto& site = plan->sites[static_cast<size_t>(id)];
    if (site.disabled) return nullptr;
    if (site.node != &n || site.op != n.op) {
      state.inactive_workspace_reuse = false;
      return nullptr;
    }
    return &site;
  }

  bool inactive_control(const Node& n, InactiveControlPlan::Site* site) {
    if (!site) return false;
    auto* plan = state.inactive_control_plan.get();
    if (!plan || site->cone >= plan->cones.size() ||
        !plan->cones[site->cone].running) {
      state.inactive_control_reuse = false;
      throw std::logic_error("inactive-control cone execution is incomplete");
    }
    auto& cone = plan->cones[site->cone];
    if (cone.decision != InactiveControlPlan::Cone::Direct) {
      finish_inactive_control_site(site);
      return false;
    }
    const Op& op = p.body.ops.at(static_cast<size_t>(n.op));
    double handles[6] = {};
    for (int k = 0; k < op.n_in; ++k) {
      handles[k] = state.bindings[static_cast<size_t>(op.in[k])];
      if (handles[k] >= 0) {
        state.inactive_control_reuse = false;
        throw std::logic_error("inactive-control activity proof failed");
      }
    }
    if (site->canonical_handle >= 0) {
      state.inactive_control_reuse = false;
      throw std::logic_error("inactive-control result handle became active");
    }
    auto& output = ref(site->canonical_handle);
    ContextLease context(*this, n, handles, false, -1, -1, output.value);
    n.forward(context.get());
    state.bindings[static_cast<size_t>(op.out)] = site->canonical_handle;
    finish_inactive_control_site(site);
    return true;
  }

  DirectControlPlan::Site* direct_control_site(const Node& n) noexcept {
    auto* plan = state.direct_control_plan.get();
    if (!plan || n.op < 0 ||
        static_cast<size_t>(n.op) >= plan->site_by_op.size())
      return nullptr;
    const int32_t site_id = plan->site_by_op[static_cast<size_t>(n.op)];
    if (site_id < 0) return nullptr;
    auto& site = plan->sites[static_cast<size_t>(site_id)];
    if (site.node != &n) return nullptr;
    return &site;
  }

  void initialize_direct_control_handles() noexcept {
    auto* plan = state.direct_control_plan.get();
    if (!plan) return;
    try {
      for (auto& site : plan->sites) {
        const Op& op = p.body.ops.at(static_cast<size_t>(site.op));
        const Slot& output = p.body.slots.at(static_cast<size_t>(op.out));
        const double canonical = state.bindings.at(static_cast<size_t>(op.out));
        const auto& canonical_ref = ref(canonical);
        if (!initial_values || canonical >= 0 || output.len != 1 ||
            output.offset < 0 || output.offset >= p.initial_size ||
            canonical_ref.value != initial_values + output.offset ||
            canonical_ref.adjoint_or_import != -1)
          throw std::logic_error(
              "direct-control canonical result handle is invalid");
        site.canonical_handle = canonical;
      }
    } catch (...) {
      // Canonical-handle setup is proof machinery, not model semantics. Fall
      // back to the ordinary retained call path for this bound state.
      state.direct_control_plan.reset();
    }
  }

  double* adj(double h, int64_t expected_len) {
    if (h < 0) return nullptr;
    if (expected_len < 0)
      throw std::logic_error("dynamic structured adjoint has negative length");
    if (is_record_scalar(h)) {
      if (expected_len != 1)
        throw std::logic_error(
            "dynamic structured record scalar has nonscalar adjoint");
      return &record_scalar(h).out2;
    }
    const auto& r = ordinary_ref(h);
    if (is_import_ref(r)) {
      const auto& import = import_ref(r);
      if (import.input < 0 || import.input >= outer.n_in || import.offset < 0)
        throw std::logic_error(
            "dynamic structured outer adjoint handle out of range");
      const int64_t at = import.offset;
      const Desc& value_desc = outer.in[import.input];
      const Desc& adj_desc = outer.in_adj[import.input];
      if (at > value_desc.len || expected_len > value_desc.len - at ||
          at > adj_desc.len || expected_len > adj_desc.len - at)
        throw std::logic_error(
            "dynamic structured outer adjoint handle out of range");
      return adj_desc.data ? adj_desc.data + at : nullptr;
    }
    const int64_t at = r.adjoint_or_import;
    if (at < 0 || at > state.adjoint_size ||
        expected_len > state.adjoint_size - at ||
        static_cast<uint64_t>(state.adjoint_size) > state.adjoints.size())
      throw std::logic_error("dynamic structured adjoint handle out of range");
    return state.adjoints.data() + at;
  }

  bool adjoint_ranges_overlap(double first_handle, int64_t first_len,
                              double second_handle, int64_t second_len) {
    if (first_handle < 0 || second_handle < 0) return false;
    if (first_len < 0 || second_len < 0)
      throw std::logic_error("structured adjoint range has negative length");
    const bool first_record = is_record_scalar(first_handle);
    const bool second_record = is_record_scalar(second_handle);
    const size_t first_id = first_record ? record_scalar_id(first_handle) : 0;
    const size_t second_id =
        second_record ? record_scalar_id(second_handle) : 0;
    if (!first_record) (void)ref(first_handle);
    if (!second_record) (void)ref(second_handle);
    if (first_record || second_record) {
      if ((first_record && first_len != 1) ||
          (second_record && second_len != 1))
        throw std::logic_error(
            "structured record-scalar adjoint has nonscalar extent");
      return first_record && second_record && first_id == second_id;
    }
    const auto& first = ref(first_handle);
    const auto& second = ref(second_handle);
    const bool first_import = is_import_ref(first);
    const bool second_import = is_import_ref(second);
    if (!first_import && !second_import) {
      const int64_t first_at = first.adjoint_or_import;
      const int64_t second_at = second.adjoint_or_import;
      if (first_at < 0 || second_at < 0 || first_at > state.adjoint_size ||
          second_at > state.adjoint_size ||
          first_len > state.adjoint_size - first_at ||
          second_len > state.adjoint_size - second_at)
        throw std::logic_error(
            "structured internal adjoint range is out of bounds");
      return first_at < second_at + second_len &&
             second_at < first_at + first_len;
    }
    if (first_import != second_import) return false;
    const auto outer_desc = [&](const DynamicLoopState::Ref& r,
                                int64_t len) -> Desc {
      const auto& import = import_ref(r);
      if (import.input < 0 || import.input >= outer.n_in || import.offset < 0)
        throw std::logic_error(
            "structured outer adjoint range is out of bounds");
      const Desc& desc = outer.in_adj[import.input];
      const int64_t at = import.offset;
      if (!desc.data || at > desc.len || len > desc.len - at)
        throw std::logic_error(
            "structured outer adjoint range is out of bounds");
      return {desc.data + at, len};
    };
    return overlaps(outer_desc(first, first_len),
                    outer_desc(second, second_len));
  }

  int64_t compact_adjoint_offset(double base_handle, int64_t base_len,
                                 int64_t output_len, const double* handles,
                                 const KernelCtx& c) {
    if (base_handle < 0 || is_record_scalar(base_handle)) return -1;
    const auto& base = ref(base_handle);
    if (is_import_ref(base) || base.adjoint_or_import < 0 || base_len <= 0 ||
        base_len != output_len)
      return -1;
    for (int k = 1; k < c.n_in; ++k) {
      if (adjoint_ranges_overlap(base_handle, base_len, handles[k],
                                 c.in[k].len))
        return -1;
      for (int j = k + 1; j < c.n_in; ++j)
        if (adjoint_ranges_overlap(handles[k], c.in[k].len, handles[j],
                                   c.in[j].len))
      return -1;
    }
    return base.adjoint_or_import;
  }

  int64_t retained_frame_values(const Node& n) const {
    const Op& op = p.body.ops[n.op];
    if (op.n_in < 0 || op.n_in > 6 || n.frame_size < 6)
      throw std::logic_error("dynamic structured frame layout is invalid");
    const int64_t values = n.frame_size - (6 - op.n_in);
    // A nonnull base keeps zero-length output and scratch descriptors valid
    // without performing pointer arithmetic on DynamicArena::allocate(0).
    return std::max<int64_t>(1, values);
  }

  static void clear_context(KernelCtx& c) noexcept {
    for (int k = 0; k < c.n_in; ++k) {
      c.in[k].data = nullptr;
      c.in_adj[k].data = nullptr;
    }
    c.out.data = nullptr;
    c.out2.data = nullptr;
    c.out_adj = 0.0;
    c.out_adj_vec.data = nullptr;
    c.out2_adj = 0.0;
    c.scratch = nullptr;
    c.eval_state = nullptr;
    c.state = nullptr;
  }

  KernelCtx& context(const Node& n, double* frame, bool backward,
                     std::array<double, 6>& inline_inputs,
                     double out_handle = -1, double out2_handle = -1,
                     double* output_override = nullptr,
                     double* output2_override = nullptr,
                     double* scratch_override = nullptr) {
    const Op& op = p.body.ops[n.op];
    if (op.n_in < 0 || op.n_in > 6)
      throw std::logic_error("dynamic structured frame arity is invalid");
    if ((output2_override != nullptr) !=
        (output_override != nullptr && op.out2 >= 0))
      throw std::logic_error("structured output overrides are inconsistent");
    if (n.op < 0 || static_cast<size_t>(n.op) >= state.context_templates.size())
      throw std::logic_error(
          "dynamic structured context template is out of range");
    if (state.cached_context_in_use)
      throw std::logic_error(
          "dynamic structured cached context is not reentrant");
    KernelCtx& c = state.context_templates[static_cast<size_t>(n.op)];
    try {
      // Templates never retain these fields between calls. Refresh every
      // per-call value even when it happens to equal the preceding call.
      c.eval_state = outer.eval_state;
      c.state = nullptr;
      for (int k = 0; k < op.n_in; ++k) {
        const double h = frame[k];
        if (is_inline_int(h)) {
          if (c.in[k].len != 1)
            throw std::logic_error(
                "dynamic structured inline integer has nonscalar input");
          int alias = -1;
          for (int j = 0; j < k; ++j)
            if (frame[j] == h && is_inline_int(frame[j])) {
              alias = j;
              break;
            }
          if (alias >= 0) {
            c.in[k].data = c.in[alias].data;
          } else {
            inline_inputs[static_cast<size_t>(k)] = inline_int_value(h);
            c.in[k].data = &inline_inputs[static_cast<size_t>(k)];
          }
        } else if (is_arena_location(h))
          c.in[k].data = state.arena.value(h);
        else if (is_record_scalar(h)) {
          if (c.in[k].len != 1)
            throw std::logic_error(
                "dynamic structured record scalar has nonscalar input");
          c.in[k].data = &record_scalar(h).out;
        } else
          c.in[k].data = ordinary_ref(h).value;
        c.in_adj[k].data = backward ? adj(h, c.in[k].len) : nullptr;
      }
      int64_t pos = op.n_in;
      c.out.data = output_override ? output_override : frame + pos;
      c.out_adj = 0.0;
      c.out_adj_vec.data = backward ? adj(out_handle, c.out.len) : nullptr;
      if (backward && c.out.len == 1) c.out_adj = c.out_adj_vec.data[0];
      pos += c.out.len;
      c.out2.data = nullptr;
      c.out2_adj = 0.0;
      if (op.out2 >= 0) {
        c.out2.data = output2_override ? output2_override : frame + pos;
        if (backward) c.out2_adj = *adj(out2_handle, c.out2.len);
        pos += c.out2.len;
      }
      c.scratch = output_override ? scratch_override : frame + pos;
    } catch (...) {
      clear_context(c);
      throw;
    }
    state.cached_context_in_use = true;
    return c;
  }

  // Kernel callbacks are synchronous, and stateful/nested body kernels are
  // rejected during preparation. One body-op template can therefore serve as
  // its call workspace. The guard both rejects unsupported same-Executor
  // reentry and restores pointer-free template state after every exception.
  struct ContextLease {
    DynamicExecution& execution;
    // context() initializes only the entries referenced by inline inputs.
    std::array<double, 6> inline_inputs;
    KernelCtx* call = nullptr;

    ContextLease(DynamicExecution& execution, const Node& n, double* frame,
                 bool backward, double out_handle = -1, double out2_handle = -1,
                 double* output_override = nullptr,
                 double* output2_override = nullptr,
                 double* scratch_override = nullptr)
        : execution(execution) {
      call = &execution.context(n, frame, backward, inline_inputs, out_handle,
                                out2_handle, output_override, output2_override,
                                scratch_override);
    }
    ContextLease(const ContextLease&) = delete;
    ContextLease& operator=(const ContextLease&) = delete;
    ~ContextLease() {
      clear_context(*call);
      execution.state.cached_context_in_use = false;
    }
    KernelCtx& get() { return *call; }
  };

  bool inactive_workspace_call(const Node& n, double* handles) {
    InactiveWorkspacePlan::Site* site = inactive_workspace_site(n);
    if (!site) return false;
    const Op& op = p.body.ops.at(static_cast<size_t>(n.op));
    const int64_t output_len = p.body.slots.at(static_cast<size_t>(op.out)).len;
    if (op.out2 >= 0 || output_len <= 0 || output_len != site->value_count) {
      state.inactive_workspace_reuse = false;
      return false;
    }
    DynamicLoopState::Ref* canonical = nullptr;
    try {
      if (!site->values) {
        site->values.reset(new double[static_cast<size_t>(output_len)]);
        state.inactive_workspace_allocated_values +=
            static_cast<uint64_t>(output_len);
      }
      if (!site->canonical_ready) {
        site->canonical_handle =
            make_ref(site->values.get(), output_len, false);
        site->canonical_ready = true;
      }
      canonical = &ref(site->canonical_handle);
      if (!canonical->value || canonical->adjoint_or_import != -1 ||
          canonical->value != site->values.get())
        throw std::logic_error("inactive-workspace output is invalid");
      const Desc output{canonical->value, output_len};
      for (int k = 0; k < op.n_in; ++k)
        if (value_overlaps(handles[k],
                           p.body.slots.at(static_cast<size_t>(op.in[k])).len,
                           output))
          return false;
    } catch (...) {
      // A failed proof guard disables this optional path before the callback
      // is invoked, so the ordinary executor remains authoritative.
      site->disabled = true;
      return false;
    }

    ContextLease context(*this, n, handles, false, -1, -1, canonical->value,
                         nullptr, state.inactive_scratch.get());
    KernelCtx& c = context.get();
    n.forward(c);
    // The plan admits only the registered resolved callback. Still fail
    // closed if its descriptor contract is ever changed: the callback has
    // already run, so materialize the produced value once instead of running
    // it a second time.
    double out = site->canonical_handle;
    bool used_workspace = true;
    uint64_t arena_locations = 0;
    if (c.out.data != canonical->value ||
        c.out.len != p.body.slots[static_cast<size_t>(op.out)].len) {
      if (!c.out.data ||
          c.out.len != p.body.slots[static_cast<size_t>(op.out)].len)
        throw std::logic_error(
            "inactive-workspace callback changed output shape");
      const DynamicArena::Allocation allocation =
          state.arena.allocate_located(c.out.len);
      std::copy_n(c.out.data, c.out.len, allocation.data);
      if (state.arena.location_handle(allocation, 0, c.out.len, out)) {
        arena_locations = 1;
      } else {
        out = make_ref(allocation.data, c.out.len, false);
      }
      used_workspace = false;
    }
    state.bindings[static_cast<size_t>(op.out)] = out;
    if (state.allocation_profile) {
      if (used_workspace) {
        state.allocation_profile->note_inactive_workspace(op, n);
      } else {
        state.allocation_profile->note_inactive_split(op, n, c.out.len,
                                                      arena_locations);
      }
    }
    return true;
  }

  bool inline_integer_result(const Node& n) {
    const Op& op = p.body.ops[static_cast<size_t>(n.op)];
    if (op.opcode != OP_COMPARE && op.opcode != OP_INT_ARITH) return false;
    const auto builtin =
        op.opcode == OP_COMPARE ? compare_forward : int_forward;
    // Only the built-in, synchronous, scalar, scratch-free callbacks have the
    // exact-int32 result contract used by the compact handle encoding.  A
    // custom callback, second output, reverse callback, or different layout
    // keeps the ordinary retained-frame behavior.
    if (n.forward != builtin || n.backward != nullptr ||
        op.out2 >= 0 || p.body.slots[static_cast<size_t>(op.out)].len != 1 ||
        n.kernel_scratch != 0)
      return false;

    double handles[6] = {};
    for (int k = 0; k < op.n_in; ++k)
      handles[k] = state.bindings[static_cast<size_t>(op.in[k])];
    double output = 0.0;
    ContextLease context(*this, n, handles, false, -1, -1, &output);
    // Invoke the resolved callback before publishing anything, preserving its
    // validation order and exact exception type/message.  OP_COMPARE returns
    // 0/1 and OP_INT_ARITH validates its result is int32 before returning.
    n.forward(context.get());
    state.bindings[static_cast<size_t>(op.out)] =
        inline_int_handle(static_cast<int32_t>(output));
    if (state.allocation_profile)
      state.allocation_profile->note_inline_integer(op,
                                                    retained_frame_values(n));
    return true;
  }

  bool direct_control(const Node& n) {
    DirectControlPlan::Site* site = direct_control_site(n);
    if (!site) return false;

    const Op& op = p.body.ops.at(static_cast<size_t>(n.op));
    double handles[6] = {};
    Desc output;
    try {
      if (site->canonical_handle >= 0)
        throw std::logic_error("direct-control result handle became active");
      output = {ref(site->canonical_handle).value, 1};
      for (int k = 0; k < op.n_in; ++k)
        handles[k] = state.bindings.at(static_cast<size_t>(op.in[k]));
      bool overlap = false;
      for (int k = 0; k < op.n_in; ++k) {
        const int64_t len = p.body.slots.at(static_cast<size_t>(op.in[k])).len;
        overlap |= value_overlaps(handles[k], len, output);
      }
      if (overlap) return false;
    } catch (...) {
      // A proof/guard invariant can only disable the optimization. The
      // ordinary frame path below remains authoritative for model behavior.
      state.direct_control_plan.reset();
      return false;
    }

    // OP_COMPARE is synchronous, scalar, scratch-free, and has no reverse
    // callback under the static proof. Invoke its already-resolved callback
    // through the same context machinery, but write the inactive canonical
    // body slot instead of retaining a per-visit frame and Ref.
    ContextLease context(*this, n, handles, false, -1, -1, output.data);
    n.forward(context.get());
    state.bindings[static_cast<size_t>(op.out)] = site->canonical_handle;
    return true;
  }

  bool compact_scalar_index(const Node& n) {
    const Op& op = p.body.ops[static_cast<size_t>(n.op)];
    if (op.opcode != OP_INDEX_DYNAMIC || op.n_in < 2 || op.out2 >= 0 ||
        n.forward != index_forward || n.backward != index_backward ||
        n.kernel_scratch != 0 ||
        p.body.slots[static_cast<size_t>(op.out)].len != 1)
      return false;
    const auto* spec = static_cast<const DynamicIndexSpec*>(op.udata);
    if (!spec || spec->selected_size != 1) return false;

    double handles[6] = {};
    bool active = false;
    for (int k = 0; k < op.n_in; ++k) {
      handles[k] = state.bindings[static_cast<size_t>(op.in[k])];
      active |= handles[k] >= 0;
    }
    if (!active) return false;

    // Keep the selected value and its adjoint inside the already-required
    // reverse record. The public handle contains only the record ordinal, so
    // vector growth cannot leave a dangling pointer. The shared helper is
    // also the built-in callback's scalar path, preserving validation and
    // exception order.
    double output = 0.0;
    ContextLease context(*this, n, handles, false, -1, -1, &output);
    const int64_t position = scalar_index_forward(context.get());
    const bool encodable_record =
        state.record_scalar_reuse &&
        state.records.size() < static_cast<uint64_t>(record_scalar_count);
    const bool encodable_position =
        position == -1 ||
        (position >= 0 && static_cast<uint64_t>(position) <=
                              DynamicLoopState::max_frame_free_position);
    if (encodable_record && encodable_position) {
      const size_t ordinal = state.records.size();
      const double out = record_scalar_handle(ordinal);
      state.conceptual_adjoint_size =
          add(state.conceptual_adjoint_size, int64_t{1});
      DynamicLoopState::Record record;
      record.base_handle = handles[0];
      record.out = output;
      record.out2 = 0.0;
      record.site = n.record_site;
      record.code = position < 0 ? DynamicLoopState::scalar_index_empty_record
                                 : static_cast<uint32_t>(position);
      state.records.push_back(record);
      state.bindings[static_cast<size_t>(op.out)] = out;
      if (state.allocation_profile)
        state.allocation_profile->note_compact_index(op, true, position < 0);
      return true;
    }

    // Preserve the H6N Ref-backed form as the exact fallback when a record
    // ordinal or reached position cannot be encoded in the compact bands.
    double* const stored_output = state.arena.allocate(1);
    stored_output[0] = output;
    const double out = make_ref(stored_output, 1, true);
    DynamicLoopState::Record record;
    record.base_handle = handles[0];
    record.out = out;
    record.out2 = static_cast<double>(position);
    record.site = n.record_site;
    record.code = DynamicLoopState::scalar_index_record;
    state.records.push_back(record);
    state.bindings[static_cast<size_t>(op.out)] = out;
    if (state.allocation_profile)
      state.allocation_profile->note_compact_index(op, false);
    return true;
  }

  bool compact_update(const Node& n) {
    if (n.compact_update_cell < 0) return false;
    const Op& op = p.body.ops[n.op];
    const auto* spec_ptr = static_cast<const DynamicIndexSpec*>(op.udata);
    IndexInputLayout layout;
    if (!spec_ptr || !index_input_layout(*spec_ptr, true, layout) ||
        op.n_in != layout.expected || op.out2 >= 0)
      throw std::logic_error("compact structured update arity is invalid");
    const auto& spec = *spec_ptr;
    const int cell = n.compact_update_cell;
    const double base_handle = state.bindings[static_cast<size_t>(op.in[0])];
    double* base = nullptr;
    if (is_inline_int(base_handle) || is_record_scalar(base_handle))
      return false;
    else if (base_handle < -static_cast<double>(ordinary_ref_limit))
      base = state.arena.value(base_handle);
    else
      base = ordinary_ref(base_handle).value;
    if (state.compact_primal_by_cell[static_cast<size_t>(cell)] != base)
      return false;
    if (n.forward != set_index_forward || n.backward != set_index_backward)
      return false;

    double handles[6] = {};
    for (int k = 0; k < op.n_in; ++k)
      handles[k] = state.bindings[static_cast<size_t>(op.in[k])];
    ContextLease context(*this, n, handles, false, -1, -1, base);
    KernelCtx& c = context.get();
    for (int k = 1; k < c.n_in; ++k)
      if (overlaps(c.out, c.in[k])) return false;
    const bool scalar_site = scalar_compact_update_spec(spec);
    bool active = false;
    for (int k = 0; k < c.n_in; ++k) active |= handles[k] >= 0;
    active &= n.backward != nullptr;
    const int64_t shared_adjoint =
        active ? compact_adjoint_offset(base_handle, c.in[0].len, c.out.len,
                                        handles, c)
               : -1;

    if (!scalar_site) {
      if (active && shared_adjoint < 0) return false;
      const int64_t selected =
          compact_ordered_update_positions(spec, c, nullptr);
      int64_t retained = mul(2, selected);
      if (active) retained = add(retained, 1);
      if (active) {
        if (retained >= retained_frame_values(n)) return false;
      } else if (add(retained, 6) >= c.out.len) {
        // Inactive ordinary calls retain only the output buffer. A compact
        // delta also adds one 16-byte Ref and one 32-byte Record, so demand a
        // strict total-byte win rather than comparing arena values alone.
        return false;
      }
      double* const delta = state.arena.allocate(retained);
      const int64_t reached = compact_ordered_update_positions(spec, c, delta);
      if (reached != selected)
        throw std::logic_error(
            "compact structured selection changed during forward");
      for (int64_t i = 0; i < selected; ++i) {
        const int64_t position = static_cast<int64_t>(delta[i]);
        delta[selected + i] = c.out.data[position];
      }
      if (active) delta[2 * selected] = handles[layout.rhs];
      const double out =
          make_ref(c.out.data, c.out.len, active, shared_adjoint);
      DynamicLoopState::Record record;
      record.frame = delta;
      record.out = out;
      record.out2 = static_cast<double>(selected);
      record.site = n.record_site;
      record.code = DynamicLoopState::delta_record;
      const bool resident_rhs = is_record_scalar(handles[layout.rhs]);
      const double resident_rhs_value =
          resident_rhs ? c.in[layout.rhs].data[0] : 0.0;
      state.records.push_back(record);
      for (int64_t i = 0; i < selected; ++i)
        c.out.data[static_cast<int64_t>(delta[i])] =
            resident_rhs ? resident_rhs_value : c.in[layout.rhs].data[i];
      state.bindings[static_cast<size_t>(op.out)] = out;
      if (state.allocation_profile)
        state.allocation_profile->note_delta_compact(op, selected, c.out.len,
                                                     active);
      return true;
    }

    const int64_t position = compact_scalar_update_position(spec, c);
    if (position < 0 || position >= c.out.len)
      throw std::logic_error("compact structured update position out of range");

    bool frame_free =
        !active || (shared_adjoint >= 0 && n.backward == set_index_backward);
    if (frame_free &&
        static_cast<uint64_t>(position) >
            DynamicLoopState::max_frame_free_position)
      frame_free = false;

    // Every allocation that can throw precedes the destructive write. A later
    // failure still drops the entire evaluation-local tape, but this ordering
    // also keeps the state internally coherent under allocation failure.
    const int retained_inputs = op.n_in;
    double* frame = nullptr;
    if (!frame_free) {
      frame = state.arena.allocate(std::max(1, retained_inputs + 1));
      std::copy_n(handles, retained_inputs, frame);
      frame[retained_inputs] = static_cast<double>(position);
    }
    bool reused_output_ref = false;
    const double out = make_compact_update_ref(
        base_handle, c.out.data, c.out.len, active, shared_adjoint, frame_free,
        reused_output_ref);
    if (shared_adjoint >= 0 && c.out.len > state.compact_adjoint_work_size)
      throw std::logic_error(
          "compact structured adjoint exceeds preallocated work");
    DynamicLoopState::Record record;
    if (frame_free && active)
      record.rhs_handle = handles[layout.rhs];
    else
      record.frame = frame;
    record.out = out;
    record.out2 = c.out.data[position];
    record.site = n.record_site;
    record.code = frame_free ? static_cast<uint32_t>(position)
                             : DynamicLoopState::retained_scalar_record;
    const double rhs_value = c.in[layout.rhs].data[0];
    state.records.push_back(record);
    c.out.data[position] = rhs_value;
    state.bindings[static_cast<size_t>(op.out)] = out;
    if (state.allocation_profile)
      state.allocation_profile->note_compact(
          op, frame_free ? 0 : std::max(1, retained_inputs + 1), c.out.len,
          active, shared_adjoint >= 0, frame_free, reused_output_ref);
    return true;
  }

  bool transient_inactive_call(const Node& n, double* handles, bool& active) {
    const Op& op = p.body.ops[static_cast<size_t>(n.op)];
    bool any_active_input = false;
    for (int k = 0; k < op.n_in; ++k) {
      handles[k] = state.bindings[static_cast<size_t>(op.in[k])];
      any_active_input |= handles[k] >= 0;
    }
    active = n.backward && any_active_input;
    if (active) return false;
    if (n.kernel_scratch > state.inactive_scratch_size ||
        (n.kernel_scratch > 0 && !state.inactive_scratch))
      return false;
    if (inactive_workspace_call(n, handles)) {
      publish_loop_invariant(n);
      return true;
    }

    const int64_t output_values =
        add(p.body.slots[static_cast<size_t>(op.out)].len,
            op.out2 >= 0 ? p.body.slots[static_cast<size_t>(op.out2)].len : 0);
    const int64_t retained_outputs = output_values;
    const DynamicArena::Allocation allocation =
        state.arena.allocate_located(output_values);
    double* outputs =
        output_values > 0 ? allocation.data : &state.inactive_empty_value;
    const int64_t primary_len = p.body.slots[static_cast<size_t>(op.out)].len;
    double* output2 = op.out2 >= 0 ? outputs + primary_len : nullptr;
    ContextLease context(*this, n, handles, false, -1, -1, outputs, output2,
                         state.inactive_scratch.get());
    KernelCtx& c = context.get();
    // The same callback runs in the same order. Only its call-local handles
    // and scratch stop entering persistent history; output storage remains in
    // the stable arena and is published after successful completion.
    n.forward(c);
    stabilize_ephemeral_aliased_output(c.out, outputs, primary_len, handles, c);
    if (op.out2 >= 0)
      stabilize_ephemeral_aliased_output(
          c.out2, output2, p.body.slots[static_cast<size_t>(op.out2)].len,
          handles, c);
    uint64_t arena_locations = 0;
    const auto output_handle = [&](double* actual, double* expected,
                                   int64_t actual_len, int64_t expected_len,
                                   size_t relative) {
      double packed = 0.0;
      if (actual == expected && actual_len == expected_len &&
          state.arena.location_handle(allocation, relative, expected_len,
                                      packed)) {
        ++arena_locations;
        return packed;
      }
      return make_ref(actual, actual_len, false);
    };
    const double out =
        output_handle(c.out.data, outputs, c.out.len, primary_len, 0);
    const double out2 =
        op.out2 >= 0
            ? output_handle(c.out2.data, output2, c.out2.len,
                            p.body.slots[static_cast<size_t>(op.out2)].len,
                            static_cast<size_t>(primary_len))
            : -1;
    state.bindings[static_cast<size_t>(op.out)] = out;
    if (op.out2 >= 0) state.bindings[static_cast<size_t>(op.out2)] = out2;
    if (n.compact_update_cell >= 0)
      state.compact_primal_by_cell[static_cast<size_t>(n.compact_update_cell)] =
          c.out.data;
    publish_loop_invariant(n);
    if (state.allocation_profile)
      state.allocation_profile->note_inactive_split(op, n, retained_outputs,
                                                    arena_locations);
    return true;
  }

  bool is_inactive_workspace_handle(double handle) const noexcept {
    const auto* plan = state.inactive_workspace_plan.get();
    if (!plan || handle >= 0) return false;
    for (const auto& site : plan->sites)
      if (site.canonical_ready && site.canonical_handle == handle) return true;
    return false;
  }

  void stabilize_ephemeral_aliased_output(Desc& output, double* stable,
                                          int64_t stable_len,
                                          const double* handles,
                                          const KernelCtx& context) {
    if (!output.data) return;
    for (int k = 0; k < context.n_in; ++k) {
      if (!overlaps(output, context.in[k])) continue;
      if (!is_record_scalar(handles[k]) && !is_inline_int(handles[k]) &&
          !is_inactive_workspace_handle(handles[k]))
        continue;
      if (output.len != stable_len)
        throw std::logic_error(
            "dynamic structured ephemeral input aliases mismatched output");
      std::copy_n(output.data, output.len, stable);
      output.data = stable;
      return;
    }
  }

  enum Flow { Normal, Break, Continue };
  Flow forward(const Node& n) {
    switch (n.kind) {
      case Node::Sequence:
        for (const auto& child : n.children) {
          const Flow flow = forward(child);
          if (flow != Normal) return flow;
        }
        return Normal;
      case Node::KernelCall: {
        const Op& op = p.body.ops[n.op];
        auto* inactive_site = inactive_control_site(n);
        begin_inactive_control_cone(inactive_site);
        if (reuse_loop_invariant(n)) {
          if (state.allocation_profile)
            state.allocation_profile->note_reuse(
                op, &DynamicAllocationProfile::Bucket::invariant_reuses);
          finish_inactive_control_site(inactive_site);
          return Normal;
        }
        if (inactive_control(n, inactive_site)) {
          if (state.allocation_profile)
            state.allocation_profile->note_reuse(
                op, &DynamicAllocationProfile::Bucket::inactive_control_calls);
          publish_loop_invariant(n);
          return Normal;
        }
        if (op.opcode == OP_COMPARE && direct_control(n)) {
          if (state.allocation_profile)
            state.allocation_profile->note_reuse(
                op, &DynamicAllocationProfile::Bucket::direct_control_calls);
          publish_loop_invariant(n);
          return Normal;
        }
        if (inline_integer_result(n)) {
          publish_loop_invariant(n);
          return Normal;
        }
        if (compact_scalar_index(n)) {
          publish_loop_invariant(n);
          return Normal;
        }
        if (compact_update(n)) return Normal;
        double handles[6] = {};
        bool active = false;
        if (transient_inactive_call(n, handles, active)) return Normal;
        const int64_t retained = retained_frame_values(n);
        double* frame = state.arena.allocate(retained);
        std::copy_n(handles, op.n_in, frame);
        ContextLease context(*this, n, frame, false);
        KernelCtx& c = context.get();
        // Preserve the kernel's exception type and message. In particular,
        // reject/domain errors and bounds errors are part of Stan's visible
        // behavior and must match the ordinary graph path.
        n.forward(c);
        // Callback output redirection may alias an input. Record storage can
        // move at the next push, so materialize that special case into the
        // stable frame before publishing a persistent Ref.
        const int64_t primary_len =
            p.body.slots[static_cast<size_t>(op.out)].len;
        stabilize_ephemeral_aliased_output(c.out, frame + op.n_in, primary_len,
                                           handles, c);
        if (op.out2 >= 0)
          stabilize_ephemeral_aliased_output(
              c.out2, frame + op.n_in + primary_len,
              p.body.slots[static_cast<size_t>(op.out2)].len, handles, c);
        const double out = make_ref(c.out.data, c.out.len, active);
        state.bindings[static_cast<size_t>(op.out)] = out;
        double out2 = -1;
        if (op.out2 >= 0)
          state.bindings[static_cast<size_t>(op.out2)] = out2 =
              make_ref(c.out2.data, c.out2.len, active);
        if (active) {
          DynamicLoopState::Record record;
          record.frame = frame;
          record.out = out;
          record.out2 = out2;
          record.site = n.record_site;
          state.records.push_back(record);
        }
        if (n.compact_update_cell >= 0) {
          state.compact_primal_by_cell[static_cast<size_t>(
              n.compact_update_cell)] = c.out.data;
        }
        publish_loop_invariant(n);
        if (state.allocation_profile)
          state.allocation_profile->note_ordinary(op, n, retained, active);
        return Normal;
      }
      case Node::Alias:
        state.bindings[static_cast<size_t>(n.dst)] =
            state.bindings[static_cast<size_t>(n.src)];
        // The destination now preserves the source's current value. Prevent a
        // later compact update from mutating that shared storage: its ordinary
        // fallback creates a fresh output and reanchors the updated binding.
        state.compact_primal_by_cell[static_cast<size_t>(n.src)] = nullptr;
        return Normal;
      case Node::If:
        return forward(n.children[scalar_value(n.condition) != 0.0 ? 0 : 1]);
      case Node::For: {
        begin_loop_invariant_scope(n);
        const double lo = scalar_value(n.lower), hi = scalar_value(n.upper);
        if (!std::isfinite(lo) || !std::isfinite(hi) || std::trunc(lo) != lo ||
            std::trunc(hi) != hi || lo < std::numeric_limits<int32_t>::min() ||
            hi < std::numeric_limits<int32_t>::min() ||
            lo > std::numeric_limits<int32_t>::max() ||
            hi > std::numeric_limits<int32_t>::max())
          throw std::logic_error("structured loop invalid integer bounds");
        const int64_t count = hi >= lo ? static_cast<int64_t>(hi - lo) + 1 : 0;
        if (count > n.capacity)
          throw std::logic_error("structured loop capacity proof failed");
        for (int64_t i = 0; i < count; ++i) {
          state.bindings[static_cast<size_t>(n.iterator)] = inline_int_handle(
              static_cast<int32_t>(static_cast<int64_t>(lo) + i));
          if (state.memory_profile) ++state.iterator_values;
          if (forward(n.children[0]) == Break) break;
        }
        return Normal;
      }
      case Node::While:
        begin_loop_invariant_scope(n);
        for (int64_t i = 0;; ++i) {
          forward(n.children[0]);
          if (scalar_value(n.condition) == 0.0) break;
          if (i == n.capacity)
            throw std::logic_error("structured while capacity proof failed");
          if (forward(n.children[1]) == Break) break;
        }
        return Normal;
      case Node::Break:
        return Break;
      case Node::Continue:
        return Continue;
      case Node::Target:
        if (state.target_refs.size() >=
            static_cast<size_t>(p.root.target_capacity))
          throw std::logic_error("structured target capacity proof failed");
        state.target_refs.push_back(state.bindings[static_cast<size_t>(n.src)]);
        return Normal;
    }
    throw std::logic_error("invalid structured node");
  }

  bool prepare_shared_compact_adjoint(const DynamicLoopState::Record& record,
                                      KernelCtx& c) {
    const Node& node = record_node(record);
    const Op& op = p.body.ops[static_cast<size_t>(node.op)];
    const auto* spec = static_cast<const DynamicIndexSpec*>(op.udata);
    IndexInputLayout layout;
    if (record.code != DynamicLoopState::retained_scalar_record ||
        record.out < 0 || !spec || !index_input_layout(*spec, true, layout) ||
        op.n_in != layout.expected || c.n_in != layout.expected)
      return false;
    const double base_handle = record.frame[0];
    if (base_handle < 0) return false;
    const auto& base = ref(base_handle);
    const auto& output = ref(record.out);
    if (is_import_ref(base) || is_import_ref(output) ||
        base.adjoint_or_import != output.adjoint_or_import)
      return false;
    if (c.in_adj[0].len != c.out_adj_vec.len || c.in_adj[0].len <= 0 ||
        !c.in_adj[0].data || !c.out_adj_vec.data ||
        c.in_adj[0].data != c.out_adj_vec.data || !state.compact_adjoint_work ||
        c.out_adj_vec.len > state.compact_adjoint_work_size)
      throw std::logic_error(
          "shared compact structured adjoint layout is invalid");
    Desc conceptual_output{state.compact_adjoint_work.get(), c.out_adj_vec.len};
    for (int k = 0; k < c.n_in; ++k) {
      if (overlaps(conceptual_output, c.in_adj[k]))
        throw std::logic_error(
            "shared compact structured adjoints are not disjoint");
      for (int j = k + 1; j < c.n_in; ++j)
        if (overlaps(c.in_adj[k], c.in_adj[j]))
      throw std::logic_error(
          "shared compact structured adjoints are not disjoint");
    }

    // The ordinary callback sees a distinct zero-initialized conceptual base
    // and the exact output-adjoint bits. Clear with +0.0, then let its existing
    // ascending loop perform every += so signed zero, NaN payloads, rounding,
    // cancellation and selected-cell RHS routing stay unchanged.
    std::copy_n(c.out_adj_vec.data, c.out_adj_vec.len, conceptual_output.data);
    std::fill_n(c.in_adj[0].data, c.in_adj[0].len, 0.0);
    c.out_adj_vec = conceptual_output;
    c.out_adj = c.out_adj_vec.len == 1 ? c.out_adj_vec.data[0] : 0.0;
    return true;
  }

  void backward_compact_scalar_index(
      const DynamicLoopState::Record& record) {
    const Node& node = record_node(record);
    if (node.op < 0 || static_cast<size_t>(node.op) >= p.body.ops.size())
      throw std::logic_error("compact scalar index node is invalid");
    const Op& op = p.body.ops[static_cast<size_t>(node.op)];
    const auto* spec = static_cast<const DynamicIndexSpec*>(op.udata);
    IndexInputLayout layout;
    if (!spec || !index_input_layout(*spec, false, layout) ||
        op.n_in != layout.expected)
      throw std::logic_error("compact scalar index record is invalid");
    const int64_t base_len = p.body.slots[static_cast<size_t>(op.in[0])].len;
    if (node.forward != index_forward || node.backward != index_backward ||
        op.opcode != OP_INDEX_DYNAMIC ||
        op.out2 >= 0 || spec->selected_size != 1 ||
        p.body.slots[static_cast<size_t>(op.out)].len != 1 ||
        node.kernel_scratch != 0 || record.out < 0 || base_len < 0 ||
        !std::isfinite(record.out2) || std::trunc(record.out2) != record.out2 ||
        record.out2 < -1 || record.out2 >= static_cast<double>(base_len))
      throw std::logic_error("compact scalar index record is invalid");

    const auto& output = ref(record.out);
    if (is_import_ref(output) || output.adjoint_or_import < 0 ||
        !output.value)
      throw std::logic_error("compact scalar index output is invalid");
    double* const base_adjoint = adj(record.base_handle, base_len);
    if (!base_adjoint || record.out2 < 0) return;
    double* const output_adjoint = adj(record.out, 1);
    if (!output_adjoint || overlaps({base_adjoint, base_len},
                                    {output_adjoint, 1}))
      throw std::logic_error("compact scalar index adjoints overlap");
    base_adjoint[static_cast<int64_t>(record.out2)] += output_adjoint[0];
  }

  void backward_record_scalar_index(const DynamicLoopState::Record& record,
                                    double output_handle) {
    const Node& node = validate_record_scalar(record);
    const Op& op = p.body.ops[static_cast<size_t>(node.op)];
    const int64_t base_len = p.body.slots[static_cast<size_t>(op.in[0])].len;
    const bool empty =
        record.code == DynamicLoopState::scalar_index_empty_record;

    double* const base_adjoint = adj(record.base_handle, base_len);
    if (!base_adjoint || empty) return;
    double* const output_adjoint = adj(output_handle, 1);
    if (!output_adjoint ||
        adjoint_ranges_overlap(record.base_handle, base_len, output_handle, 1))
      throw std::logic_error("record scalar index adjoints overlap");
    base_adjoint[record.code] += output_adjoint[0];
  }

  void backward_delta_compact(const DynamicLoopState::Record& record) {
    const Node& node = record_node(record);
    if (node.op < 0 || static_cast<size_t>(node.op) >= p.body.ops.size() ||
        !std::isfinite(record.out2) || std::trunc(record.out2) != record.out2 ||
        record.out2 < 0 || record.out2 > static_cast<double>(exact_limit))
      throw std::logic_error("delta compact structured record is invalid");
    const Op& op = p.body.ops[static_cast<size_t>(node.op)];
    const auto* spec = static_cast<const DynamicIndexSpec*>(op.udata);
    IndexInputLayout layout;
    const int64_t selected = static_cast<int64_t>(record.out2);
    const int64_t output_len = p.body.slots[static_cast<size_t>(op.out)].len;
    if (!spec || node.forward != set_index_forward ||
        node.backward != set_index_backward ||
        op.opcode != OP_SET_INDEX_DYNAMIC ||
        !index_input_layout(*spec, true, layout) ||
        op.n_in != layout.expected || op.out2 >= 0 ||
        selected > spec->selected_size || output_len <= 0 ||
        (selected > 0 && !record.frame))
      throw std::logic_error("delta compact structured record is invalid");
    const int64_t rhs_len =
        p.body.slots[static_cast<size_t>(op.in[layout.rhs])].len;
    if (rhs_len != spec->selected_size)
      throw std::logic_error("delta compact structured record is invalid");

    double* const delta = record.frame;
    int64_t previous = -1;
    for (int64_t i = 0; i < selected; ++i) {
      const double raw = delta[i];
      if (!std::isfinite(raw) || std::trunc(raw) != raw || raw < 0 ||
          raw >= static_cast<double>(output_len) ||
          static_cast<int64_t>(raw) <= previous)
        throw std::logic_error(
            "delta compact structured positions are invalid");
      previous = static_cast<int64_t>(raw);
    }

    auto& output = ref(record.out);
    if (!output.value)
      throw std::logic_error("delta compact structured output is unavailable");
    if (record.out >= 0) {
      if (is_import_ref(output) || output.adjoint_or_import < 0 || !delta)
        throw std::logic_error(
            "delta compact structured adjoint layout is invalid");
      const double rhs_handle = delta[2 * selected];
      double* const shared_adjoint = adj(record.out, output_len);
      double* const rhs_adjoint = adj(rhs_handle, rhs_len);
      if (!shared_adjoint ||
          adjoint_ranges_overlap(record.out, output_len, rhs_handle, rhs_len))
        throw std::logic_error(
            "delta compact structured adjoints are not disjoint");

      int64_t ordinal = 0;
      int64_t position = selected > 0 ? static_cast<int64_t>(delta[0]) : -1;
      for (int64_t i = 0; i < output_len; ++i) {
        const double contribution = shared_adjoint[i];
        shared_adjoint[i] = 0.0;
        if (ordinal < selected && i == position) {
          if (rhs_adjoint) rhs_adjoint[ordinal] += contribution;
          ++ordinal;
          if (ordinal < selected)
            position = static_cast<int64_t>(delta[ordinal]);
        } else {
          shared_adjoint[i] += contribution;
        }
      }
      if (ordinal != selected)
        throw std::logic_error(
            "delta compact structured selection is unordered");
    }

    for (int64_t i = selected; i-- > 0;)
      output.value[static_cast<int64_t>(delta[i])] = delta[selected + i];
  }

  void backward_frame_free_compact(const DynamicLoopState::Record& record,
                                   int64_t position) {
    const Node& node = record_node(record);
    const Op& op = p.body.ops[static_cast<size_t>(node.op)];
    const auto* spec = static_cast<const DynamicIndexSpec*>(op.udata);
    IndexInputLayout layout;
    const int64_t output_len = p.body.slots[static_cast<size_t>(op.out)].len;
    if (record.out < 0 || node.backward != set_index_backward ||
        op.opcode != OP_SET_INDEX_DYNAMIC || !spec ||
        !index_input_layout(*spec, true, layout) ||
        op.n_in != layout.expected || op.out2 >= 0 ||
        p.body.slots[static_cast<size_t>(op.in[layout.rhs])].len != 1 ||
        position < 0 || position >= output_len || output_len <= 0 ||
        output_len > state.compact_adjoint_work_size ||
        !state.compact_adjoint_work)
      throw std::logic_error("frame-free compact structured record is invalid");
    auto& output = ref(record.out);
    if (is_import_ref(output) || output.adjoint_or_import < 0)
      throw std::logic_error(
          "frame-free compact structured output is not internal");
    if (!output.value)
      throw std::logic_error(
          "frame-free compact structured output is unavailable");
    double* const shared_adjoint = adj(record.out, output_len);
    double* const rhs_adjoint = adj(record.rhs_handle, 1);
    if (!shared_adjoint)
      throw std::logic_error(
          "frame-free compact structured adjoint is unavailable");

    double* const conceptual_output = state.compact_adjoint_work.get();
    if (adjoint_ranges_overlap(record.out, output_len, record.rhs_handle, 1) ||
        overlaps({conceptual_output, output_len},
                 {shared_adjoint, output_len}) ||
        (rhs_adjoint &&
         overlaps({conceptual_output, output_len}, {rhs_adjoint, 1})))
      throw std::logic_error(
          "frame-free compact structured adjoints are not disjoint");
    // The saved forward position is the tape value. The built-in scalar
    // update has no selector adjoint; its reverse behavior depends only on
    // that position, the RHS adjoint, and the ordered output-adjoint scan.
    std::copy_n(shared_adjoint, output_len, conceptual_output);
    std::fill_n(shared_adjoint, output_len, 0.0);
    for (int64_t i = 0; i < output_len; ++i) {
      if (i == position) {
        if (rhs_adjoint) rhs_adjoint[0] += conceptual_output[i];
      } else {
        shared_adjoint[i] += conceptual_output[i];
      }
    }
  }

  void backward() {
    for (size_t i = state.records.size(); i-- > 0;) {
      const auto& record = state.records[i];
      const Node& node = record_node(record);
      if (node.op < 0 || static_cast<size_t>(node.op) >= p.body.ops.size())
        throw std::logic_error("dynamic structured record node is invalid");
      if (record.code == DynamicLoopState::delta_record) {
        backward_delta_compact(record);
      } else if (record.code == DynamicLoopState::scalar_index_record) {
        backward_compact_scalar_index(record);
      } else if (p.body.ops[static_cast<size_t>(node.op)].opcode ==
                     OP_INDEX_DYNAMIC &&
                 (record.code == DynamicLoopState::scalar_index_empty_record ||
                  record.code <= DynamicLoopState::max_frame_free_position)) {
        backward_record_scalar_index(record, record_scalar_handle(i));
      } else if (record.code != DynamicLoopState::ordinary_record) {
        const bool frame_free =
            record.code != DynamicLoopState::retained_scalar_record;
        double raw_position = frame_free
                                  ? static_cast<double>(record.code)
                                  : record.frame[p.body.ops[node.op].n_in];
        if (!std::isfinite(raw_position) ||
            std::trunc(raw_position) != raw_position || raw_position < 0 ||
            raw_position > static_cast<double>(exact_limit))
          throw std::logic_error(
              "compact structured undo position is invalid");
        const int64_t position = static_cast<int64_t>(raw_position);
        auto& output = ref(record.out);
        const int64_t output_len = p.body.slots[p.body.ops[node.op].out].len;
        if (position < 0 || position >= output_len)
          throw std::logic_error(
              "compact structured undo position out of range");
        if (record.out >= 0 && node.backward) {
          if (frame_free) {
            backward_frame_free_compact(record, position);
          } else {
            ContextLease context(*this, node, record.frame, true, record.out,
                                 -1, output.value);
            KernelCtx& c = context.get();
            prepare_shared_compact_adjoint(record, c);
            node.backward(c);
          }
        }
        output.value[position] = record.out2;
      } else if (node.backward) {
        ContextLease context(*this, node, record.frame, true, record.out,
                             record.out2);
        KernelCtx& c = context.get();
        node.backward(c);
      }
    }
  }
};

int64_t scratch_size(const Op& op, const Slot*) {
  const auto& p = *static_cast<const StructuredLoop*>(op.udata);
  return p.dynamic_history ? 0 : p.scratch_size;
}

KernelState* make_loop_state(const Op& op, const Slot*) {
  const auto& p = *static_cast<const StructuredLoop*>(op.udata);
  return p.dynamic_history ? new DynamicLoopState(p) : nullptr;
}

void dynamic_loop_forward(KernelCtx& ctx) {
  DynamicExecution e(ctx);
  const auto& p = e.p;
  auto& s = e.state;
  if (s.cached_context_in_use)
    throw std::logic_error("dynamic structured execution is not reentrant");
  // A failed replacement forward must never leave either the preceding tape
  // or a partially built replacement resident/available to reverse.
  s.release_tape();
  s.memory_profile = std::getenv("STANLI_STRUCTURED_MEMORY_PROFILE") != nullptr;
  if (s.memory_profile) {
    try {
      s.allocation_profile = std::make_unique<DynamicAllocationProfile>();
    } catch (...) {
      // Profiling is observational. Failure to allocate its counters cannot
      // reduce model coverage or change the exception seen by the caller.
      s.allocation_profile.reset();
    }
  }
  if (s.loop_invariant_plan) s.loop_invariant_plan->reset_runtime();
  s.loop_invariant_reuse =
      s.loop_invariant_plan &&
      std::getenv("STANLI_NO_STRUCTURED_INVARIANT_REUSE") == nullptr;
  if (s.inactive_control_plan) s.inactive_control_plan->reset_runtime();
  s.inactive_control_reuse =
      s.inactive_control_plan &&
      std::getenv("STANLI_NO_STRUCTURED_INACTIVE_CONTROL") == nullptr;
  if (s.inactive_workspace_plan) s.inactive_workspace_plan->reset_runtime();
  s.inactive_workspace_reuse =
      s.inactive_workspace_plan &&
      std::getenv("STANLI_NO_STRUCTURED_INACTIVE_WORKSPACE") == nullptr;
  s.record_scalar_reuse =
      std::getenv("STANLI_NO_STRUCTURED_RECORD_SCALARS") == nullptr;
  s.shared_update_ref_reuse =
      std::getenv("STANLI_NO_STRUCTURED_SHARED_UPDATE_REFS") == nullptr;
  struct ReleaseOnFailure {
    DynamicLoopState& state;
    bool published = false;
    ~ReleaseOnFailure() {
      if (!published) state.release_tape();
    }
  } release_on_failure{s};
  int64_t expected =
      p.has_target ? 1 + (p.target_fragment ? p.root.target_capacity : 0) : 0;
  for (int slot : p.outputs) expected += p.body.slots[slot].len;
  if (expected != ctx.out.len)
    throw std::logic_error("dynamic structured output size mismatch");

  double* initial = s.arena.allocate(p.initial_size);
  if (s.allocation_profile)
    s.allocation_profile->initial_arena_values =
        static_cast<uint64_t>(p.initial_size);
  e.initial_values = initial;
  std::fill_n(initial, p.initial_size, 0.0);
  s.bindings.resize(p.body.slots.size());
  s.compact_primal_by_cell.assign(p.body.slots.size(), nullptr);
  s.records.clear();
  s.target_refs.clear();
  for (size_t slot = 0; slot < p.body.slots.size(); ++slot)
    s.bindings[slot] = e.make_ref(initial + p.body.slots[slot].offset,
                                  p.body.slots[slot].len, false);
  if (s.allocation_profile)
    s.allocation_profile->initial_slot_refs = s.refs.size();
  e.initialize_direct_control_handles();
  e.initialize_inactive_control_handles();
  if (s.allocation_profile) {
    if (s.refs.size() < s.allocation_profile->initial_slot_refs) {
      s.allocation_profile->overflow = true;
    } else {
      s.allocation_profile->canonical_refs =
          s.refs.size() - s.allocation_profile->initial_slot_refs;
    }
  }
  for (const auto& fill : p.fills)
    std::copy(fill.second.begin(), fill.second.end(),
              initial + p.body.slots[fill.first].offset);
  for (size_t import_ordinal = 0; import_ordinal < p.imports.size();
       ++import_ordinal) {
    const auto& in = p.imports[import_ordinal];
    const Slot& slot = p.body.slots[in.slot];
    if (in.input >= ctx.n_in || in.offset > ctx.in[in.input].len ||
        slot.len > ctx.in[in.input].len - in.offset)
      throw std::logic_error("dynamic structured import exceeds graph input");
    s.bindings[static_cast<size_t>(in.slot)] =
        e.make_import_ref(initial + slot.offset, slot.len,
                          ctx.in_adj[in.input].data != nullptr, import_ordinal);
    std::copy_n(ctx.in[in.input].data + in.offset, slot.len,
                initial + slot.offset);
  }
  if (s.allocation_profile) {
    uint64_t before_imports = s.allocation_profile->initial_slot_refs;
    s.allocation_profile->add(before_imports,
                              s.allocation_profile->canonical_refs);
    if (s.refs.size() < before_imports) {
      s.allocation_profile->overflow = true;
    } else {
      s.allocation_profile->import_refs = s.refs.size() - before_imports;
    }
  }

  e.forward(p.root);
  int64_t pos = 0;
  for (int slot : p.outputs) {
    e.copy_value(s.bindings[static_cast<size_t>(slot)], p.body.slots[slot].len,
                 ctx.out.data + pos);
    pos += p.body.slots[slot].len;
  }
  if (p.has_target) {
    if (p.target_fragment) {
      ctx.out.data[pos++] = static_cast<double>(s.target_refs.size());
      for (size_t i = 0; i < s.target_refs.size(); ++i)
        ctx.out.data[pos + static_cast<int64_t>(i)] =
            e.scalar_value(s.target_refs[i]);
      std::fill(ctx.out.data + pos + static_cast<int64_t>(s.target_refs.size()),
                ctx.out.data + pos + p.root.target_capacity, 0.0);
      pos += p.root.target_capacity;
    } else {
      s.target_work.resize(s.target_refs.size());
      for (size_t i = 0; i < s.target_refs.size(); ++i)
        s.target_work[i] = e.scalar_value(s.target_refs[i]);
      size_t count = s.target_work.size();
      while (count > 1) {
        size_t next = 0;
        for (size_t i = 0; i < count; i += 6) {
          if (i + 1 == count) {
            s.target_work[next++] = s.target_work[i];
            continue;
          }
          double sum = 0;
          for (size_t j = i; j < std::min(count, i + 6); ++j)
            sum += s.target_work[j];
          s.target_work[next++] = sum;
        }
        count = next;
      }
      ctx.out.data[pos++] = count ? s.target_work[0] : 0.0;
    }
  }
  if (pos != ctx.out.len)
    throw std::logic_error("dynamic structured output size mismatch");
  if (s.memory_profile) report_memory_profile(p, s);
  s.reverse_ready = true;
  release_on_failure.published = true;
}

void dynamic_loop_backward(KernelCtx& ctx) {
  DynamicExecution e(ctx);
  const auto& p = e.p;
  auto& s = e.state;
  if (s.cached_context_in_use)
    throw std::logic_error("dynamic structured execution is not reentrant");
  if (!s.reverse_ready)
    throw std::logic_error(
        "dynamic structured reverse has no successful forward state");
  // Consume before seeding any adjoints.  Both a successful reverse and one
  // that throws partway through leave the tape unavailable; recovery requires
  // a fresh, successful forward sweep.
  s.reverse_ready = false;
  struct ReleaseAfterReverse {
    DynamicLoopState& state;
    ~ReleaseAfterReverse() { state.release_tape(); }
  } release_after_reverse{s};
  s.adjoints.assign(static_cast<size_t>(s.adjoint_size), 0.0);
  int64_t pos = 0;
  for (int slot : p.outputs) {
    if (double* adj = e.adj(s.bindings[static_cast<size_t>(slot)],
                            p.body.slots[slot].len))
      for (int64_t i = 0; i < p.body.slots[slot].len; ++i)
        adj[i] += ctx.out_adj_vec.data[pos + i];
    pos += p.body.slots[slot].len;
  }
  if (p.has_target) {
    for (size_t i = 0; i < s.target_refs.size(); ++i)
      if (double* adj = e.adj(s.target_refs[i], 1))
        *adj += ctx.out_adj_vec
                    .data[pos + (p.target_fragment ? 1 + static_cast<int64_t>(i)
                                                   : 0)];
  }
  e.backward();
}

void validate_target(const TargetReduction& p, int64_t input_size) {
  int64_t total = 0;
  for (const auto& s : p.sources) {
    if (s.offset < 0 || s.capacity < 0 || s.offset > input_size ||
        int64_t(s.fragment) > input_size - s.offset ||
        s.capacity > input_size - s.offset - int64_t(s.fragment) ||
        (!s.fragment && s.capacity != 1))
      throw std::logic_error("target fragment exceeds packed input");
    total = add(total, s.capacity);
  }
  if (total != p.capacity)
    throw std::logic_error("target fragment capacity mismatch");
}
int64_t target_scratch(const Op& op, const Slot* slots) {
  const auto& p = *static_cast<const TargetReduction*>(op.udata);
  if (op.n_in != 1 || slots[op.out].len != 1)
    throw std::logic_error("invalid target reducer shape");
  validate_target(p, slots[op.in[0]].len);
  return add(p.capacity, static_cast<int64_t>(p.sources.size()));
}
void target_forward(KernelCtx& c) {
  const auto& p = *static_cast<const TargetReduction*>(c.udata);
  if (c.n_in != 1 || c.out.len != 1)
    throw std::logic_error("invalid target reducer shape");
  validate_target(p, c.in[0].len);
  int64_t size = 0;
  for (size_t k = 0; k < p.sources.size(); ++k) {
    const auto& s = p.sources[k];
    int64_t count = s.capacity;
    if (s.fragment) {
      const double n = c.in[0].data[s.offset];
      if (!std::isfinite(n) || n < 0 || n > s.capacity || std::trunc(n) != n)
        throw std::logic_error("invalid target fragment count");
      count = static_cast<int64_t>(n);
    }
    c.scratch[p.capacity + k] = static_cast<double>(count);
    std::copy_n(c.in[0].data + s.offset + s.fragment, count, c.scratch + size);
    size += count;
  }
  // Reduce exactly the reached leaves, without padding or a per-region sum.
  while (size > 1) {
    int64_t next = 0;
    for (int64_t i = 0; i < size; i += 6) {
      if (i + 1 == size) {
        c.scratch[next++] = c.scratch[i];
        continue;
      }
      double sum = 0;
      for (int64_t j = i; j < std::min(size, i + 6); ++j) sum += c.scratch[j];
      c.scratch[next++] = sum;
    }
    size = next;
  }
  c.out.data[0] = size ? c.scratch[0] : 0.0;
}
void target_backward(KernelCtx& c) {
  if (!c.in_adj[0].data) return;
  const auto& p = *static_cast<const TargetReduction*>(c.udata);
  validate_target(p, c.in[0].len);
  for (size_t k = 0; k < p.sources.size(); ++k) {
    const double count = c.scratch[p.capacity + k];
    if (!std::isfinite(count) || count < 0 || count > p.sources[k].capacity ||
        std::trunc(count) != count)
      throw std::logic_error("invalid saved target fragment count");
  }
  for (size_t k = 0; k < p.sources.size(); ++k) {
    const auto& s = p.sources[k];
    const int64_t count = static_cast<int64_t>(c.scratch[p.capacity + k]);
    for (int64_t i = 0; i < count; ++i)
      c.in_adj[0].data[s.offset + s.fragment + i] += c.out_adj;
  }
}

void compare_forward(KernelCtx& c) {
  const double a = c.in[0].data[0];
  const double b = c.n_in > 1 ? c.in[1].data[0] : 0;
  bool value;
  switch (c.variant) {
    case 0:
      value = a < b;
      break;
    case 1:
      value = a <= b;
      break;
    case 2:
      value = a > b;
      break;
    case 3:
      value = a >= b;
      break;
    case 4:
      value = a == b;
      break;
    case 5:
      value = a != b;
      break;
    case 6:
      value = a == 0;
      break;
    case 7:
      value = std::isnan(a);
      break;
    case 8:
      value = std::isinf(a);
      break;
    default:
      throw std::logic_error("invalid comparison variant");
  }
  c.out.data[0] = value ? 1 : 0;
}
int64_t integer(double x) {
  if (!std::isfinite(x) || std::trunc(x) != x ||
      x < std::numeric_limits<int32_t>::min() ||
      x > std::numeric_limits<int32_t>::max())
    throw std::domain_error("integer arithmetic exceeds Stan integer range");
  return static_cast<int64_t>(x);
}
void int_forward(KernelCtx& c) {
  const int64_t a = integer(c.in[0].data[0]);
  const int64_t b = c.n_in > 1 ? integer(c.in[1].data[0]) : 0;
  int64_t v;
  switch (c.variant) {
    case 0:
      v = a + b;
      break;
    case 1:
      v = a - b;
      break;
    case 2:
      v = a * b;
      break;
    case 3:
      if (b == 0) throw std::domain_error("integer division by zero");
      v = a / b;
      break;
    case 4:
      if (b == 0) throw std::domain_error("integer remainder by zero");
      v = a % b;
      break;
    case 5:
      v = -a;
      break;
    default:
      throw std::logic_error("invalid integer arithmetic variant");
  }
  c.out.data[0] = static_cast<double>(integer(static_cast<double>(v)));
}

const Desc& index_input(const KernelCtx& c, int input, const char* what) {
  if (input < 0 || input >= c.n_in)
    throw std::logic_error(std::string("invalid ") + what + " input");
  return c.in[input];
}

int64_t index_integer(const KernelCtx& c, int input, int64_t offset,
                      int64_t upper, const char* what) {
  const Desc& values = index_input(c, input, what);
  if (offset < 0 || offset >= values.len)
    throw std::logic_error(std::string("invalid ") + what + " offset");
  const double raw = values.data[offset];
  if (!std::isfinite(raw) || std::trunc(raw) != raw || raw < 0 ||
      raw > static_cast<double>(upper))
    throw std::out_of_range(std::string(what) + " exceeds capacity");
  return static_cast<int64_t>(raw);
}

int64_t logical_axis_extent(const DynamicIndexSpec::Axis& axis,
                            const KernelCtx& c) {
  return axis.extent_input_offset < 0
             ? axis.extent
             : index_integer(c, axis.extent_input, axis.extent_input_offset,
                             axis.extent, "dynamic index logical extent");
}

int64_t dynamic_axis_count(const DynamicIndexSpec::Axis& axis,
                           const KernelCtx& c) {
  if (axis.count_input_offset < 0) return axis.count;
  const int64_t upper = axis.kind == DynamicIndexSpec::Axis::Range
                            ? std::numeric_limits<int32_t>::max()
                            : axis.count;
  int64_t count = index_integer(c, axis.count_input, axis.count_input_offset,
                                upper, "dynamic index count");
  if (axis.kind == DynamicIndexSpec::Axis::Range) {
    const Desc& selector =
        index_input(c, axis.selector_input, "dynamic range start");
    if (axis.input_offset < 0 || axis.input_offset >= selector.len)
      throw std::logic_error("invalid dynamic range start offset");
    const double first = selector.data[axis.input_offset];
    if (!std::isfinite(first) || std::trunc(first) != first || first < 1 ||
        first > std::numeric_limits<int32_t>::max())
      throw std::domain_error("dynamic range start is not an integer");
    count = std::max<int64_t>(0, count - static_cast<int64_t>(first) + 1);
  }
  if (count < 0 || count > axis.count)
    throw std::out_of_range("dynamic index count " + std::to_string(count) +
                            " exceeds capacity " + std::to_string(axis.count));
  return count;
}

struct IndexRuntime {
  // Stan indices are normally low-dimensional. Keep their validation state
  // inline, while retaining arbitrary-rank support with one contiguous heap
  // block instead of one allocation for each field.
  static constexpr size_t inline_dimensions = 8;

  explicit IndexRuntime(size_t dimensions) : dimensions_(dimensions) {
    if (dimensions_ > inline_dimensions) {
      if (dimensions_ > std::numeric_limits<size_t>::max() / 3)
        throw std::length_error("dynamic index rank exceeds storage limit");
      heap_storage_.reset(new int64_t[3 * dimensions_]);
      storage_ = heap_storage_.get();
    } else {
      storage_ = inline_storage_.data();
    }
  }
  IndexRuntime(const IndexRuntime&) = delete;
  IndexRuntime& operator=(const IndexRuntime&) = delete;
  IndexRuntime(IndexRuntime&& other) noexcept
      : heap_storage_(std::move(other.heap_storage_)),
        dimensions_(other.dimensions_),
        selected(other.selected) {
    if (heap_storage_) {
      storage_ = heap_storage_.get();
    } else {
      std::copy_n(other.inline_storage_.data(), 3 * dimensions_,
                  inline_storage_.data());
      storage_ = inline_storage_.data();
    }
  }

  int64_t& extent(size_t dim) { return storage_[dim]; }
  int64_t extent(size_t dim) const { return storage_[dim]; }
  int64_t& count(size_t dim) { return storage_[dimensions_ + dim]; }
  int64_t count(size_t dim) const { return storage_[dimensions_ + dim]; }
  int64_t& stride(size_t dim) { return storage_[2 * dimensions_ + dim]; }
  int64_t stride(size_t dim) const { return storage_[2 * dimensions_ + dim]; }

 private:
  std::array<int64_t, 3 * inline_dimensions> inline_storage_;
  std::unique_ptr<int64_t[]> heap_storage_;
  int64_t* storage_ = nullptr;
  size_t dimensions_;

 public:
  int64_t selected = 1;
};

int64_t selected_position(const DynamicIndexSpec& p, const KernelCtx& c,
                          const IndexRuntime& runtime, int64_t linear) {
  int64_t result = 0;
  const auto consume = [&](size_t dim, int64_t& q) {
    const auto& axis = p.axes[dim];
    const int64_t count = runtime.count(dim);
    const int64_t ordinal = q % count;
    q /= count;
    double raw;
    switch (axis.kind) {
      case DynamicIndexSpec::Axis::All:
        raw = static_cast<double>(ordinal + 1);
        break;
      case DynamicIndexSpec::Axis::Single:
        raw = index_input(c, axis.selector_input, "structured selector")
                  .data[axis.input_offset];
        break;
      case DynamicIndexSpec::Axis::Multi:
        raw = index_input(c, axis.selector_input, "structured selector")
                  .data[axis.input_offset + ordinal];
        break;
      case DynamicIndexSpec::Axis::Range:
        raw = index_input(c, axis.selector_input, "structured selector")
                  .data[axis.input_offset] +
              static_cast<double>(ordinal);
        break;
      default:
        throw std::logic_error("invalid index selector");
    }
    if (!std::isfinite(raw) || std::trunc(raw) != raw || raw < 1 ||
        raw > static_cast<double>(runtime.extent(dim)))
      throw std::out_of_range("structured index out of range");
    result += (static_cast<int64_t>(raw) - 1) * runtime.stride(dim);
  };
  const size_t outer = p.axes.size() - (p.matrix_leaf ? 2 : 0);
  if (p.matrix_leaf) {
    consume(outer, linear);
    consume(outer + 1, linear);
  }
  for (size_t d = outer; d-- > 0;) consume(d, linear);
  return result;
}
IndexRuntime validate_index(const DynamicIndexSpec& p, const KernelCtx& c,
                            bool update) {
  const IndexInputLayout layout = require_index_input_layout(p, update);
  if (c.n_in != layout.expected || (p.matrix_leaf && p.axes.size() < 2))
    throw std::logic_error("invalid structured index descriptor");
  const auto validate_selector_input = [&](int input, const char* what) {
    if (input < 1 || input >= layout.selector_end)
      throw std::logic_error(std::string("invalid ") + what + " input");
  };
  IndexRuntime runtime(p.axes.size());
  int64_t capacity = 1, logical_size = 1;
  for (size_t dim = 0; dim < p.axes.size(); ++dim) {
    const auto& axis = p.axes[dim];
    if (axis.kind != DynamicIndexSpec::Axis::All)
      validate_selector_input(axis.selector_input, "structured selector");
    if (axis.count_input_offset >= 0)
      validate_selector_input(axis.count_input, "dynamic index count");
    if (axis.extent_input_offset >= 0)
      validate_selector_input(axis.extent_input,
                              "dynamic index logical extent");
    const int64_t logical_extent = logical_axis_extent(axis, c);
    runtime.extent(dim) = logical_extent;
    const int64_t count = dynamic_axis_count(axis, c);
    runtime.count(dim) = count;
    runtime.selected = mul(runtime.selected, count);
    capacity = mul(capacity, axis.extent);
    logical_size = mul(logical_size, logical_extent);
    if (axis.stride < 0 ||
        (axis.kind != DynamicIndexSpec::Axis::All && axis.input_offset < 0))
      throw std::logic_error("invalid structured index offset");
    const int64_t width =
        axis.kind == DynamicIndexSpec::Axis::All     ? 0
        : axis.kind == DynamicIndexSpec::Axis::Multi ? axis.count
        : axis.kind == DynamicIndexSpec::Axis::Range &&
                axis.count_input_offset >= 0 &&
                axis.count_input == axis.selector_input &&
                axis.count_input_offset == axis.input_offset + 1
            ? 2
            : 1;
    const Desc* selector =
        axis.kind == DynamicIndexSpec::Axis::All
            ? nullptr
            : &index_input(c, axis.selector_input, "structured selector");
    if ((selector && (axis.input_offset > selector->len ||
                      width > selector->len - axis.input_offset)) ||
        (axis.count_input_offset >= 0 &&
         axis.count_input_offset >=
             index_input(c, axis.count_input, "dynamic index count").len) ||
        (axis.extent_input_offset >= 0 &&
         axis.extent_input_offset >=
             index_input(c, axis.extent_input, "dynamic index logical extent")
                 .len) ||
        (axis.kind == DynamicIndexSpec::Axis::Single && axis.count != 1) ||
        (axis.kind == DynamicIndexSpec::Axis::All && axis.count != axis.extent))
      throw std::logic_error("invalid structured index shape");
    // Validate selectors even when a different axis makes the result empty.
    if (axis.kind == DynamicIndexSpec::Axis::All ||
        (axis.kind == DynamicIndexSpec::Axis::Range && count == 0))
      continue;
    const int64_t selector_width =
        axis.kind == DynamicIndexSpec::Axis::Multi ? count : 1;
    for (int64_t j = 0; j < selector_width; ++j) {
      const double raw = selector->data[axis.input_offset + j];
      const double last = axis.kind == DynamicIndexSpec::Axis::Range
                              ? raw + static_cast<double>(count - 1)
                              : raw;
      if (!std::isfinite(raw) || std::trunc(raw) != raw || raw < 1 ||
          last > logical_extent)
        throw std::out_of_range("structured index out of range");
    }
  }
  const size_t outer = p.axes.size() - (p.matrix_leaf ? 2 : 0);
  int64_t capacity_stride = 1;
  int64_t logical_stride = 1;
  if (p.matrix_leaf) {
    if (p.axes[outer].stride != 1 ||
        p.axes[outer + 1].stride != p.axes[outer].extent)
      throw std::logic_error("invalid structured matrix stride");
    runtime.stride(outer) = 1;
    runtime.stride(outer + 1) = runtime.extent(outer);
    capacity_stride = mul(p.axes[outer].extent, p.axes[outer + 1].extent);
    logical_stride = mul(runtime.extent(outer), runtime.extent(outer + 1));
  }
  for (size_t d = outer; d-- > 0;) {
    if (p.axes[d].stride != capacity_stride)
      throw std::logic_error("invalid structured array stride");
    runtime.stride(d) = logical_stride;
    capacity_stride = mul(capacity_stride, p.axes[d].extent);
    logical_stride = mul(logical_stride, runtime.extent(d));
  }
  if (capacity != c.in[0].len || logical_size > capacity ||
      runtime.selected > p.selected_size ||
      (update
           ? (c.out.len != capacity || c.in[layout.rhs].len != p.selected_size)
              : c.out.len != p.selected_size))
    throw std::logic_error("invalid structured index storage");
  return runtime;
}
int64_t compact_ordered_update_positions(const DynamicIndexSpec& p,
                                         const KernelCtx& c,
                                         double* positions) {
  if (!index_selection_is_ordered_unique(p))
    throw std::logic_error("compact structured update selection is unordered");
  const IndexRuntime runtime = validate_index(p, c, true);
  int64_t previous = -1;
  for (int64_t i = 0; i < runtime.selected; ++i) {
    const int64_t position = selected_position(p, c, runtime, i);
    if (position <= previous || position > exact_limit)
      throw std::logic_error(
          "compact structured update positions are not ordered and unique");
    if (positions) positions[i] = static_cast<double>(position);
    previous = position;
  }
  return runtime.selected;
}
int64_t compact_scalar_update_position(const DynamicIndexSpec& p,
                                       const KernelCtx& c) {
  const IndexRuntime runtime = validate_index(p, c, true);
  if (runtime.selected != 1)
    throw std::logic_error("compact structured update is not scalar");
  return selected_position(p, c, runtime, 0);
}
int64_t scalar_index_forward(KernelCtx& c) {
  const auto& p = *static_cast<const DynamicIndexSpec*>(c.udata);
  const IndexRuntime runtime = validate_index(p, c, false);
  if (p.selected_size != 1 || c.out.len != 1 || runtime.selected > 1)
    throw std::logic_error("invalid compact scalar index shape");
  c.out.data[0] = 0.0;
  if (runtime.selected == 0) return -1;
  const int64_t position = selected_position(p, c, runtime, 0);
  c.out.data[0] = c.in[0].data[position];
  return position;
}
void index_forward(KernelCtx& c) {
  const auto& p = *static_cast<const DynamicIndexSpec*>(c.udata);
  if (p.selected_size == 1 && c.out.len == 1) {
    (void)scalar_index_forward(c);
    return;
  }
  const IndexRuntime runtime = validate_index(p, c, false);
  std::fill(c.out.data, c.out.data + c.out.len, 0.0);
  for (int64_t i = 0; i < runtime.selected; ++i)
    c.out.data[i] = c.in[0].data[selected_position(p, c, runtime, i)];
}
void index_backward(KernelCtx& c) {
  if (!c.in_adj[0].data) return;
  const auto& p = *static_cast<const DynamicIndexSpec*>(c.udata);
  const IndexRuntime runtime = validate_index(p, c, false);
  for (int64_t i = 0; i < runtime.selected; ++i)
    c.in_adj[0].data[selected_position(p, c, runtime, i)] +=
        c.out_adj_vec.data[i];
}
void set_index_forward(KernelCtx& c) {
  const auto& p = *static_cast<const DynamicIndexSpec*>(c.udata);
  const IndexInputLayout layout = require_index_input_layout(p, true);
  const IndexRuntime runtime = validate_index(p, c, true);
  std::copy_n(c.in[0].data, c.in[0].len, c.out.data);
  const bool may_repeat = !index_selection_is_ordered_unique(p);
  if (may_repeat) std::fill(c.scratch, c.scratch + c.in[0].len, -1.0);
  for (int64_t i = 0; i < runtime.selected; ++i) {
    const int64_t at = selected_position(p, c, runtime, i);
    c.out.data[at] = c.in[layout.rhs].data[i];
    if (may_repeat)
      c.scratch[at] =
          static_cast<double>(i);  // last write wins for duplicate indices
  }
}
void set_index_backward(KernelCtx& c) {
  const auto& p = *static_cast<const DynamicIndexSpec*>(c.udata);
  const IndexInputLayout layout = require_index_input_layout(p, true);
  if (c.n_in != layout.expected)
    throw std::logic_error("invalid structured index descriptor");
  if (index_selection_is_ordered_unique(p)) {
    const IndexRuntime runtime = validate_index(p, c, true);
    int64_t selected = 0;
    int64_t selected_at =
        runtime.selected > 0 ? selected_position(p, c, runtime, 0) : -1;
    for (int64_t i = 0; i < c.out.len; ++i) {
      if (selected < runtime.selected && i == selected_at) {
        if (c.in_adj[layout.rhs].data)
          c.in_adj[layout.rhs].data[selected] += c.out_adj_vec.data[i];
        ++selected;
        if (selected < runtime.selected)
          selected_at = selected_position(p, c, runtime, selected);
      } else if (c.in_adj[0].data) {
        c.in_adj[0].data[i] += c.out_adj_vec.data[i];
      }
    }
    if (selected != runtime.selected)
      throw std::logic_error("unordered structured index selection");
    return;
  }
  for (int64_t i = 0; i < c.out.len; ++i) {
    const int64_t selected = static_cast<int64_t>(c.scratch[i]);
    if (selected < 0) {
      if (c.in_adj[0].data) c.in_adj[0].data[i] += c.out_adj_vec.data[i];
    } else if (c.in_adj[layout.rhs].data) {
      c.in_adj[layout.rhs].data[selected] += c.out_adj_vec.data[i];
    }
  }
}
int64_t set_index_scratch(const Op& op, const Slot* slots) {
  const auto& p = *static_cast<const DynamicIndexSpec*>(op.udata);
  return index_selection_is_ordered_unique(p) ? 0 : slots[op.in[0]].len;
}
}  // namespace

void StructuredLoop::prepare(int64_t max_bytes) {
  if (max_bytes < 0)
    throw std::length_error("negative structured history limit");
  initial_size = 0;
  for (auto& s : body.slots) {
    s.offset = initial_size;
    initial_size = add(initial_size, s.len);
  }
  for (const auto& f : fills) {
    slot(*this, f.first);
    if (static_cast<int64_t>(f.second.size()) != body.slots[f.first].len)
      throw std::invalid_argument("structured fill size mismatch");
  }
  for (const auto& in : imports) {
    slot(*this, in.slot);
    if (in.input < 0 || in.input >= 6 || in.offset < 0)
      throw std::invalid_argument("invalid structured import");
  }
  for (int s : outputs) slot(*this, s);
  node_count = record_node_count = compact_update_sites = 0;
  prepare_node(*this, root, 0, 0);
  classify_compact_updates(*this);
  bindings_offset = initial_size;
  history_offset =
      add(add(initial_size, static_cast<int64_t>(body.slots.size())), 1);
  primal_size = add(history_offset, root.frame_size);
  target_refs_offset = primal_size;
  target_work_offset = add(target_refs_offset, root.target_capacity);
  adjoint_offset = add(target_work_offset, root.target_capacity);
  scratch_size = add(adjoint_offset, primal_size);
  if (scratch_size > max_bytes / static_cast<int64_t>(sizeof(double)))
    throw std::length_error("structured loop full history exceeds budget");
  body.compact_idata();
}

void structured_loop_forward(KernelCtx& ctx) {
  const auto& plan = *static_cast<const StructuredLoop*>(ctx.udata);
  if (plan.dynamic_history) {
    dynamic_loop_forward(ctx);
    return;
  }
  Execution e(ctx);
  const auto& p = e.p;
  int64_t expected =
      p.has_target ? 1 + (p.target_fragment ? p.root.target_capacity : 0) : 0;
  for (int s : p.outputs) expected += p.body.slots[s].len;
  if (expected != ctx.out.len)
    throw std::logic_error("structured output size mismatch");
  std::fill(e.arena, e.arena + p.initial_size, 0.0);
  for (size_t s = 0; s < p.body.slots.size(); ++s)
    e.bindings[s] = handle(p.body.slots[s].offset, false);
  for (const auto& f : p.fills)
    std::copy(f.second.begin(), f.second.end(), e.value(f.first));
  for (const auto& in : p.imports) {
    const Slot& s = p.body.slots[in.slot];
    if (in.input >= ctx.n_in || in.offset > ctx.in[in.input].len ||
        s.len > ctx.in[in.input].len - in.offset)
      throw std::logic_error("structured import exceeds graph input");
    e.bindings[in.slot] =
        handle(s.offset, ctx.in_adj[in.input].data != nullptr);
    std::copy_n(ctx.in[in.input].data + in.offset, s.len, e.arena + s.offset);
  }
  e.forward(p.root, p.history_offset);
  e.arena[p.history_offset - 1] = static_cast<double>(e.targets);
  int64_t pos = 0;
  for (int s : p.outputs) {
    std::copy_n(e.value(s), p.body.slots[s].len, ctx.out.data + pos);
    pos += p.body.slots[s].len;
  }
  if (p.has_target) {
    if (p.target_fragment) {
      ctx.out.data[pos++] = static_cast<double>(e.targets);
      for (int64_t i = 0; i < e.targets; ++i)
        ctx.out.data[pos + i] =
            e.arena[offset(e.arena[p.target_refs_offset + i])];
      std::fill(ctx.out.data + pos + e.targets,
                ctx.out.data + pos + p.root.target_capacity, 0.0);
      pos += p.root.target_capacity;
    } else {
      double* work = e.arena + p.target_work_offset;
      for (int64_t i = 0; i < e.targets; ++i)
        work[i] = e.arena[offset(e.arena[p.target_refs_offset + i])];
      int64_t count = e.targets;
      while (count > 1) {
        int64_t next = 0;
        for (int64_t i = 0; i < count; i += 6) {
          if (i + 1 == count) {
            work[next++] = work[i];
            continue;
          }
          double sum = 0;
          for (int64_t j = i; j < std::min(count, i + 6); ++j) sum += work[j];
          work[next++] = sum;
        }
        count = next;
      }
      ctx.out.data[pos++] = count ? work[0] : 0.0;
    }
  }
  if (pos != ctx.out.len)
    throw std::logic_error("structured output size mismatch");
}

void structured_loop_backward(KernelCtx& ctx) {
  const auto& plan = *static_cast<const StructuredLoop*>(ctx.udata);
  if (plan.dynamic_history) {
    dynamic_loop_backward(ctx);
    return;
  }
  Execution e(ctx);
  const auto& p = e.p;
  std::fill(e.arena + p.adjoint_offset,
            e.arena + p.adjoint_offset + p.primal_size, 0.0);
  int64_t pos = 0;
  for (int s : p.outputs) {
    if (double* a = e.adj(e.bindings[s]))
      for (int64_t i = 0; i < p.body.slots[s].len; ++i)
        a[i] += ctx.out_adj_vec.data[pos + i];
    pos += p.body.slots[s].len;
  }
  if (p.has_target) {
    const int64_t count = static_cast<int64_t>(e.arena[p.history_offset - 1]);
    for (int64_t i = 0; i < count; ++i)
      if (double* a = e.adj(e.arena[p.target_refs_offset + i]))
        *a += ctx.out_adj_vec.data[pos + (p.target_fragment ? 1 + i : 0)];
  }
  e.backward(p.root, p.history_offset);
}

void register_structured_loop_kernel() {
  register_kernel(OP_TARGET_REDUCE,
                  {target_forward, target_backward, target_scratch});
  register_kernel(OP_LOOP, {structured_loop_forward, structured_loop_backward,
                            scratch_size, make_loop_state});
  register_kernel(OP_COMPARE, {compare_forward, nullptr, nullptr});
  register_kernel(OP_INT_ARITH, {int_forward, nullptr, nullptr});
  register_kernel(OP_INDEX_DYNAMIC, {index_forward, index_backward, nullptr});
  register_kernel(OP_SET_INDEX_DYNAMIC,
                  {set_index_forward, set_index_backward, set_index_scratch});
}
}  // namespace stanli
