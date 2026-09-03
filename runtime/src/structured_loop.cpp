#include <stanli/structured_loop.hpp>
#include <stanli/optable.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace stanli {
namespace {

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

void slot(const StructuredLoop& p, int s) {
  if (s < 0 || static_cast<size_t>(s) >= p.body.slots.size())
    throw std::invalid_argument("structured loop invalid slot");
}
void scalar(const StructuredLoop& p, int s) {
  slot(p, s);
  if (p.body.slots[s].len != 1)
    throw std::invalid_argument("structured loop control needs a scalar");
}
int64_t length(const StructuredLoop& p, int s) {
  return s < 0 ? 0 : p.body.slots[static_cast<size_t>(s)].len;
}

int update_rhs(const Op& op) {
  const auto* spec = static_cast<const DynamicIndexSpec*>(op.udata);
  IndexInputLayout layout;
  if (op.opcode != OP_SET_INDEX_DYNAMIC || !spec ||
      !index_input_layout(*spec, true, layout) || op.n_in != layout.expected)
    return -1;
  return op.in[layout.rhs];
}

void prepare_node(StructuredLoop& p, Node& n, unsigned depth,
                  unsigned loop_depth, std::vector<char>& out_seen) {
  if (depth > 256) throw std::length_error("structured loop nesting limit");
  ++p.node_count;
  if (n.storage != Node::InPlace) n.storage = Node::Retained;
  n.active = false;
  n.memo = false;
  n.trace = false;
  n.invariant_loop = -1;
  n.memo_index = -1;
  n.trace_index = -1;
  n.memo_outs.clear();
  n.memo_fresh = 0;
  n.site = ~uint32_t{0};
  n.workspace = -1;
  n.loop_index = -1;
  const bool loop = n.kind == Node::For || n.kind == Node::While;
  if (loop) {
    if (p.loop_count >= static_cast<size_t>(std::numeric_limits<int>::max()))
      throw std::length_error("too many structured loops");
    n.loop_index = static_cast<int>(p.loop_count++);
  }
  switch (n.kind) {
    case Node::Sequence:
      break;
    case Node::KernelCall: {
      if (p.site_count >= std::numeric_limits<uint32_t>::max())
        throw std::length_error("too many structured kernel call sites");
      n.site = static_cast<uint32_t>(p.site_count++);
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
      if (out_seen[op.out] || (op.out2 >= 0 && out_seen[op.out2]) ||
          op.out2 == op.out)
        throw std::invalid_argument("structured kernel outputs must be unique");
      out_seen[op.out] = 1;
      if (op.out2 >= 0) out_seen[op.out2] = 1;
      const Kernel* k = find_kernel(op.opcode);
      if (!k)
        throw std::invalid_argument("unregistered structured body kernel");
      if (k->make_state)
        throw std::invalid_argument(
            "stateful structured body kernel is unsupported");
      if (!n.forward) n.forward = k->forward;
      if (!n.backward) n.backward = k->backward;
      n.kernel_scratch =
          k->scratch_size ? k->scratch_size(op, p.body.slots.data()) : 0;
      if (n.kernel_scratch < 0)
        throw std::invalid_argument("negative structured kernel scratch");
      if (n.storage == Node::InPlace &&
          (update_rhs(op) < 0 || length(p, op.out) != length(p, op.in[0])))
        throw std::invalid_argument("invalid in-place structured update");
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
      break;
    case Node::For:
      scalar(p, n.lower);
      scalar(p, n.upper);
      scalar(p, n.iterator);
      if (n.children.size() != 1)
        throw std::invalid_argument("invalid structured for");
      break;
    case Node::While:
      scalar(p, n.condition);
      if (n.children.size() != 2)
        throw std::invalid_argument("invalid structured while");
      break;
    case Node::Break:
    case Node::Continue:
      if (!loop_depth) throw std::invalid_argument("unbound structured exit");
      break;
    case Node::Target:
      scalar(p, n.src);
      if (!p.has_target)
        throw std::invalid_argument("structured target needs a target output");
      break;
  }
  for (auto& c : n.children)
    prepare_node(p, c, depth + 1, loop_depth + loop, out_seen);
}

template <class F>
void walk(Node& n, std::vector<int>& loops, F& f) {
  f(n, loops);
  const bool loop = n.kind == Node::For || n.kind == Node::While;
  if (loop) loops.push_back(n.loop_index);
  for (auto& c : n.children) walk(c, loops, f);
  if (loop) loops.pop_back();
}

struct SlotUses {
  std::vector<int> kernel, alias, target, control;
  std::vector<char> output;
  explicit SlotUses(const StructuredLoop& p)
      : kernel(p.body.slots.size(), 0),
        alias(p.body.slots.size(), 0),
        target(p.body.slots.size(), 0),
        control(p.body.slots.size(), 0),
        output(p.body.slots.size(), 0) {
    for (int s : p.outputs) output[s] = 1;
  }
};

void fuse_updates(StructuredLoop& p, Node& n, SlotUses& uses) {
  if (n.kind == Node::Sequence) {
    for (size_t i = 0; i + 1 < n.children.size(); ++i) {
      Node& k = n.children[i];
      const Node& next = n.children[i + 1];
      if (k.kind != Node::KernelCall || k.storage == Node::InPlace) continue;
      const Op& op = p.body.ops[k.op];
      const int rhs = update_rhs(op);
      const int o = op.out, base = op.in[0];
      if (rhs < 0 || op.out2 >= 0 || next.kind != Node::Alias ||
          next.dst != base || next.src != o || length(p, o) != length(p, base))
        continue;
      bool base_reused = false;
      for (int j = 1; j < op.n_in; ++j) base_reused |= op.in[j] == base;
      if (base_reused || uses.kernel[o] != 0 || uses.alias[o] != 1 ||
          uses.target[o] != 0 || uses.control[o] != 0 || uses.output[o])
        continue;
      k.storage = Node::InPlace;
      --uses.alias[o];
      n.children.erase(n.children.begin() + static_cast<ptrdiff_t>(i + 1));
    }
  }
  for (auto& c : n.children) fuse_updates(p, c, uses);
}

struct Memoizer {
  StructuredLoop& p;
  const SlotUses& uses;
  std::vector<char> param_dep, escape;
  std::vector<int> traced;
  bool changed = false;

  Memoizer(StructuredLoop& plan, const SlotUses& slot_uses)
      : p(plan),
        uses(slot_uses),
        param_dep(plan.body.slots.size(), 0),
        escape(plan.loop_count, 0),
        traced(plan.body.slots.size(), 0) {
    for (const auto& in : p.imports)
      if (!in.data_only) param_dep[in.slot] = 1;
  }

  void mark(std::vector<char>& set, int i) {
    if (!set[i]) set[i] = changed = true;
  }
  bool controlled(const Node& n) const {
    switch (n.kind) {
      case Node::If:
        return param_dep[n.condition];
      case Node::For:
        return param_dep[n.lower] || param_dep[n.upper] || escape[n.loop_index];
      case Node::While:
        return param_dep[n.condition] || escape[n.loop_index];
      default:
        return false;
    }
  }

  void propagate(const Node& n, bool ctrl, int loop) {
    switch (n.kind) {
      case Node::KernelCall: {
        const Op& op = p.body.ops[n.op];
        bool any = ctrl;
        for (int k = 0; k < op.n_in; ++k) any |= param_dep[op.in[k]] != 0;
        if (!any) return;
        if (n.storage == Node::InPlace) {
          mark(param_dep, op.in[0]);
        } else {
          mark(param_dep, op.out);
          if (op.out2 >= 0) mark(param_dep, op.out2);
        }
        return;
      }
      case Node::Alias:
        if (ctrl || param_dep[n.src]) mark(param_dep, n.dst);
        return;
      case Node::Break:
      case Node::Continue:
        if (ctrl) mark(escape, loop);
        return;
      case Node::Target:
        return;
      case Node::Sequence:
        for (const auto& c : n.children) propagate(c, ctrl, loop);
        return;
      case Node::If:
        for (const auto& c : n.children)
          propagate(c, ctrl || param_dep[n.condition], loop);
        return;
      case Node::For: {
        const bool inner = ctrl || controlled(n);
        if (inner) mark(param_dep, n.iterator);
        propagate(n.children[0], inner, n.loop_index);
        return;
      }
      case Node::While: {
        const bool inner = ctrl || controlled(n);
        for (const auto& c : n.children) propagate(c, inner, n.loop_index);
        return;
      }
    }
  }

  bool memoizable(const Node& n, int loops) const {
    switch (n.kind) {
      case Node::Sequence:
        return std::all_of(n.children.begin(), n.children.end(),
                           [&](const Node& c) { return memoizable(c, loops); });
      case Node::KernelCall: {
        const Op& op = p.body.ops[n.op];
        if (n.storage == Node::InPlace || is_effectful_op(op.opcode) ||
            param_dep[op.out] || (op.out2 >= 0 && param_dep[op.out2]))
          return false;
        for (int k = 0; k < op.n_in; ++k)
          if (param_dep[op.in[k]]) return false;
        return true;
      }
      case Node::Alias:
        return !param_dep[n.src] && !param_dep[n.dst];
      case Node::If:
        return !controlled(n) && memoizable(n.children[0], loops) &&
               memoizable(n.children[1], loops);
      case Node::For:
        return !controlled(n) && !param_dep[n.iterator] &&
               memoizable(n.children[0], loops + 1);
      case Node::While:
        return !controlled(n) && memoizable(n.children[0], loops + 1) &&
               memoizable(n.children[1], loops + 1);
      case Node::Break:
      case Node::Continue:
        return loops > 0;
      case Node::Target:
        return false;
    }
    return false;
  }

  int outside(int s, const std::vector<int>& inside) const {
    return uses.kernel[s] + uses.alias[s] + uses.control[s] + uses.target[s] -
           inside[s];
  }
  bool traceable(const Node& n) const {
    if (controlled(n)) return false;
    if (n.kind == Node::If) return true;
    if (n.kind != Node::While || !memoizable(n.children[0], 1)) return false;
    std::vector<int> inside(p.body.slots.size(), 0), written;
    reads(n.children[0], inside);
    writes(n.children[0], written);
    return std::none_of(written.begin(), written.end(), [&](int s) {
      return uses.output[s] || outside(s, inside) > (s == n.condition ? 1 : 0);
    });
  }

  void group(Node& n, bool ok) {
    if (n.kind == Node::If || n.kind == Node::For || n.kind == Node::While) {
      if (ok && memoizable(n, 0)) {
        n.memo = true;
        return;
      }
      if (ok && traceable(n)) {
        if (p.trace_count >=
            static_cast<size_t>(std::numeric_limits<int>::max()))
          throw std::length_error("too many structured trace nodes");
        n.trace = true;
        n.trace_index = static_cast<int>(p.trace_count++);
        ++traced[n.condition];
      }
      for (auto& c : n.children) group(c, ok && !controlled(n));
      return;
    }
    if (n.kind != Node::Sequence) return;
    std::vector<Node> grouped, run;
    const auto flush = [&] {
      if (run.empty()) return;
      Node block;
      block.memo = true;
      block.children = std::move(run);
      grouped.push_back(std::move(block));
      ++p.node_count;
      run.clear();
    };
    for (auto& c : n.children) {
      if ((c.kind == Node::KernelCall || c.kind == Node::Alias) && ok &&
          memoizable(c, 0)) {
        run.push_back(std::move(c));
        continue;
      }
      flush();
      group(c, ok);
      grouped.push_back(std::move(c));
    }
    flush();
    if (grouped.size() == 1 && grouped[0].kind == Node::Sequence &&
        grouped[0].memo) {
      n.memo = true;
      n.children = std::move(grouped[0].children);
      --p.node_count;
    } else {
      n.children = std::move(grouped);
    }
  }

  void reads(const Node& n, std::vector<int>& count) const {
    switch (n.kind) {
      case Node::KernelCall: {
        const Op& op = p.body.ops[n.op];
        for (int k = 0; k < op.n_in; ++k) ++count[op.in[k]];
        break;
      }
      case Node::Alias:
        ++count[n.src];
        break;
      case Node::Target:
        ++count[n.src];
        break;
      case Node::If:
      case Node::While:
        ++count[n.condition];
        break;
      case Node::For:
        ++count[n.lower];
        ++count[n.upper];
        break;
      default:
        break;
    }
    for (const auto& c : n.children) reads(c, count);
  }
  void writes(const Node& n, std::vector<int>& out) const {
    switch (n.kind) {
      case Node::KernelCall: {
        const Op& op = p.body.ops[n.op];
        out.push_back(op.out);
        if (op.out2 >= 0) out.push_back(op.out2);
        break;
      }
      case Node::Alias:
        out.push_back(n.dst);
        break;
      case Node::For:
        out.push_back(n.iterator);
        break;
      default:
        break;
    }
    for (const auto& c : n.children) writes(c, out);
  }
  void number(Node& n) {
    if (n.memo) {
      if (p.memo_count >= static_cast<size_t>(std::numeric_limits<int>::max()))
        throw std::length_error("too many structured memo nodes");
      n.memo_index = static_cast<int>(p.memo_count++);
      std::vector<int> inside(p.body.slots.size(), 0), written;
      reads(n, inside);
      writes(n, written);
      std::sort(written.begin(), written.end());
      written.erase(std::unique(written.begin(), written.end()), written.end());
      for (int s : written)
        if (uses.output[s] || outside(s, inside) > traced[s])
          n.memo_outs.push_back(s);
      return;
    }
    for (auto& c : n.children) number(c);
  }

  void run() {
    do {
      changed = false;
      propagate(p.root, false, -1);
    } while (changed);
    group(p.root, true);
    number(p.root);
  }
};

void classify(StructuredLoop& p) {
  const size_t slots = p.body.slots.size();
  SlotUses uses(p);
  std::vector<int> loops;
  auto count_uses = [&](Node& n, const std::vector<int>&) {
    switch (n.kind) {
      case Node::KernelCall: {
        const Op& op = p.body.ops[n.op];
        for (int k = 0; k < op.n_in; ++k) ++uses.kernel[op.in[k]];
        break;
      }
      case Node::Alias:
        ++uses.alias[n.src];
        break;
      case Node::Target:
        ++uses.target[n.src];
        break;
      case Node::If:
      case Node::While:
        ++uses.control[n.condition];
        break;
      case Node::For:
        ++uses.control[n.lower];
        ++uses.control[n.upper];
        break;
      default:
        break;
    }
  };
  walk(p.root, loops, count_uses);
  fuse_updates(p, p.root, uses);
  Memoizer(p, uses).run();

  std::vector<char> active(slots, 0);
  for (const auto& in : p.imports)
    if (in.active) active[in.slot] = 1;
  bool changed = true;
  auto propagate = [&](Node& n, const std::vector<int>&) {
    if (n.kind == Node::Alias) {
      if (active[n.src] && !active[n.dst]) active[n.dst] = changed = true;
      return;
    }
    if (n.kind != Node::KernelCall || !n.backward) return;
    const Op& op = p.body.ops[n.op];
    if (n.storage == Node::InPlace) {
      const int base = op.in[0];
      if (active[update_rhs(op)] && !active[base])
        active[base] = changed = true;
      return;
    }
    bool any = false;
    for (int k = 0; k < op.n_in; ++k) any |= active[op.in[k]] != 0;
    if (!any) return;
    if (!active[op.out]) active[op.out] = changed = true;
    if (op.out2 >= 0 && !active[op.out2]) active[op.out2] = changed = true;
  };
  while (changed) {
    changed = false;
    walk(p.root, loops, propagate);
  }
  auto mark_active = [&](Node& n, const std::vector<int>&) {
    if (n.kind != Node::KernelCall || !n.backward) return;
    const Op& op = p.body.ops[n.op];
    if (n.storage == Node::InPlace) {
      n.active = active[op.in[0]] || active[update_rhs(op)];
      return;
    }
    for (int k = 0; k < op.n_in; ++k) n.active |= active[op.in[k]] != 0;
  };
  walk(p.root, loops, mark_active);

  std::vector<std::vector<char>> written(p.loop_count,
                                         std::vector<char>(slots, 0));
  auto mark_written = [&](Node& n, const std::vector<int>& enclosing) {
    const auto write = [&](int s) {
      for (int loop : enclosing) written[loop][s] = 1;
    };
    switch (n.kind) {
      case Node::KernelCall: {
        const Op& op = p.body.ops[n.op];
        if (n.storage == Node::InPlace) {
          write(op.in[0]);
        } else {
          write(op.out);
          if (op.out2 >= 0) write(op.out2);
        }
        break;
      }
      case Node::Alias:
        write(n.dst);
        break;
      case Node::For:
        write(n.iterator);
        written[n.loop_index][n.iterator] = 1;
        break;
      default:
        break;
    }
  };
  walk(p.root, loops, mark_written);

  std::vector<char> inplace_base(slots, 0), active_reader(slots, 0);
  auto mark_invariant = [&](Node& n, const std::vector<int>& enclosing) {
    if (n.kind != Node::KernelCall) return;
    const Op& op = p.body.ops[n.op];
    if (n.storage == Node::InPlace) inplace_base[op.in[0]] = 1;
    if (n.active)
      for (int k = 0; k < op.n_in; ++k) active_reader[op.in[k]] = 1;
    if (n.storage == Node::InPlace || is_effectful_op(op.opcode)) return;
    for (int loop : enclosing) {
      bool varies = false;
      for (int k = 0; k < op.n_in; ++k) varies |= written[loop][op.in[k]] != 0;
      if (!varies) {
        n.invariant_loop = loop;
        return;
      }
    }
  };
  walk(p.root, loops, mark_invariant);

  const auto retained = [&](int s) {
    return uses.alias[s] || uses.target[s] || uses.output[s] ||
           inplace_base[s] || active_reader[s];
  };
  auto split_memo_outs = [&](Node& n, const std::vector<int>&) {
    if (!n.memo) return;
    n.memo_fresh =
        static_cast<size_t>(std::stable_partition(n.memo_outs.begin(),
                                                  n.memo_outs.end(), retained) -
                            n.memo_outs.begin());
  };
  walk(p.root, loops, split_memo_outs);

  auto classify_transient = [&](Node& n, const std::vector<int>&) {
    if (n.kind == Node::For) {
      if (retained(n.iterator)) return;
      n.storage = Node::Transient;
      n.workspace = p.workspace_size;
      p.workspace_size = add(p.workspace_size, 1);
      return;
    }
    if (n.kind != Node::KernelCall || n.active || n.storage == Node::InPlace)
      return;
    const Op& op = p.body.ops[n.op];
    if (retained(op.out) || (op.out2 >= 0 && retained(op.out2))) return;
    n.storage = Node::Transient;
    n.workspace = p.workspace_size;
    p.workspace_size =
        add(p.workspace_size,
            add(add(length(p, op.out), length(p, op.out2)), n.kernel_scratch));
  };
  walk(p.root, loops, classify_transient);
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

template <class F>
void selected_positions(const DynamicIndexSpec& p, const KernelCtx& c,
                        const IndexRuntime& runtime, F&& f) {
  for (int64_t i = 0; i < runtime.selected; ++i)
    f(i, selected_position(p, c, runtime, i));
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
  selected_positions(p, c, runtime, [&](int64_t i, int64_t at) {
    c.out.data[i] = c.in[0].data[at];
  });
}
void index_backward(KernelCtx& c) {
  if (!c.in_adj[0].data) return;
  const auto& p = *static_cast<const DynamicIndexSpec*>(c.udata);
  const IndexRuntime runtime = validate_index(p, c, false);
  selected_positions(p, c, runtime, [&](int64_t i, int64_t at) {
    c.in_adj[0].data[at] += c.out_adj_vec.data[i];
  });
}
void set_index_forward(KernelCtx& c) {
  const auto& p = *static_cast<const DynamicIndexSpec*>(c.udata);
  const IndexInputLayout layout = require_index_input_layout(p, true);
  const IndexRuntime runtime = validate_index(p, c, true);
  std::copy_n(c.in[0].data, c.in[0].len, c.out.data);
  const bool may_repeat = !index_selection_is_ordered_unique(p);
  if (may_repeat) std::fill(c.scratch, c.scratch + c.in[0].len, -1.0);
  selected_positions(p, c, runtime, [&](int64_t i, int64_t at) {
    c.out.data[at] = c.in[layout.rhs].data[i];
    if (may_repeat) c.scratch[at] = static_cast<double>(i);
  });
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

struct BlockArena {
  struct Block {
    std::unique_ptr<double[]> data;
    size_t capacity = 0;
    size_t used = 0;
  };
  static constexpr size_t min_block = size_t{1} << 16;
  std::vector<Block> blocks;
  size_t total = 0;

  static Block make(size_t capacity) {
    Block block;
    block.data.reset(new double[capacity]);
    block.capacity = capacity;
#ifndef NDEBUG
    std::fill_n(block.data.get(), capacity,
                std::numeric_limits<double>::quiet_NaN());
#endif
    return block;
  }
  double* allocate(int64_t count) {
    if (count < 0 ||
        static_cast<uint64_t>(count) > std::numeric_limits<size_t>::max() / 2)
      throw std::length_error("structured loop storage overflow");
    const size_t n = static_cast<size_t>(count);
    if (blocks.empty() || n > blocks.back().capacity - blocks.back().used) {
      const size_t grown = blocks.empty() ? 0 : blocks.back().capacity * 2;
      const size_t capacity = std::max({n, min_block, grown});
      blocks.push_back(make(capacity));
      total += capacity;
    }
    Block& block = blocks.back();
    double* result = block.data.get() + block.used;
    block.used += n;
    return result;
  }
  void clear() {
    if (blocks.size() > 1) {
      const size_t capacity = total;
      blocks.clear();
      blocks.push_back(make(capacity));
    } else if (!blocks.empty()) {
      blocks.back().used = 0;
    }
  }
};

struct Version {
  double* value;
  int64_t adjoint;  // >= 0 offset; -1 inactive; <= -2 import -(adjoint + 2)
};

struct Record {
  enum Kind : uint8_t { Kernel, InPlace, Copy } kind;
  uint32_t site;
  int64_t handles;  // Kernel: first saved input version; InPlace: undo offset
  int64_t out;      // Kernel: out version; InPlace: base; Copy: from
  int64_t out2;     // Kernel: out2 version or -1; InPlace: rhs; Copy: to
  int64_t count;    // InPlace: written positions; Copy: length
  double* scratch;
};

struct LoopState : KernelState {
  const StructuredLoop& p;
  BlockArena arena;
  std::vector<double> workspace;
  std::vector<Version> versions;
  std::vector<int64_t> bindings;
  std::vector<int64_t> handles;
  std::vector<double> undo;
  std::vector<Record> records;
  std::vector<int64_t> target_refs;
  std::vector<int32_t> owner;
  std::vector<int64_t> node_generation, node_version, node_version2;
  std::vector<int64_t> loop_generation, loop_version;
  std::vector<KernelCtx> ctx;
  std::vector<const Node*> sites;
  std::vector<uint32_t> transient_sites;
  std::vector<const Node*> transient_loops;
  std::vector<double> adjoints;
  std::vector<double> target_work;
  std::vector<std::vector<double>> memo_tape;
  std::vector<int64_t> memo_entries, memo_stride, memo_ordinal;
  std::vector<const Node*> memo_nodes;
  std::vector<int64_t> memo_shared_base, memo_shared;
  std::vector<std::vector<uint8_t>> trace_arms;
  std::vector<std::vector<int64_t>> trace_counts;
  std::vector<int64_t> trace_ordinal;
  size_t memo_restores = 0;
  int64_t adjoint_size = 0;
  bool reverse_ready = false;
  bool memo_ready = false;
  bool report_tape =
      std::getenv("STANLI_STRUCTURED_LOOP_DIAGNOSTICS") != nullptr;

  explicit LoopState(const StructuredLoop& plan)
      : p(plan),
        workspace(static_cast<size_t>(plan.workspace_size), 0.0),
        node_generation(plan.site_count, -1),
        node_version(plan.site_count, -1),
        node_version2(plan.site_count, -1),
        loop_generation(plan.loop_count, 0),
        loop_version(plan.loop_count, -1),
        ctx(plan.site_count),
        sites(plan.site_count, nullptr),
        memo_tape(plan.memo_count),
        memo_entries(plan.memo_count, 0),
        memo_stride(plan.memo_count, 0),
        memo_ordinal(plan.memo_count, 0),
        memo_nodes(plan.memo_count, nullptr),
        memo_shared_base(plan.memo_count, 0),
        trace_arms(plan.trace_count),
        trace_counts(plan.trace_count),
        trace_ordinal(plan.trace_count, 0) {
    collect(plan.root);
    int64_t shared = 0;
    for (size_t m = 0; m < memo_nodes.size(); ++m) {
      const Node* n = memo_nodes[m];
      if (!n) throw std::logic_error("structured loop memo numbering is stale");
      memo_shared_base[m] = shared;
      shared += static_cast<int64_t>(n->memo_outs.size() - n->memo_fresh);
    }
    memo_shared.assign(static_cast<size_t>(shared), -1);
    for (size_t site = 0; site < sites.size(); ++site) {
      const Node* n = sites[site];
      if (!n) throw std::logic_error("structured loop site numbering is stale");
      const Op& op = p.body.ops[n->op];
      KernelCtx& c = ctx[site];
      c.n_in = op.n_in;
      c.variant = op.variant;
      c.idata = op.idata;
      c.n_idata = op.n_idata;
      c.udata = op.udata;
      for (int k = 0; k < op.n_in; ++k) {
        const int64_t len = p.body.slots[op.in[k]].len;
        c.in[k] = Desc{nullptr, len};
        c.in_adj[k] = Desc{nullptr, len};
      }
      c.out = Desc{nullptr, p.body.slots[op.out].len};
      c.out_adj_vec = Desc{nullptr, p.body.slots[op.out].len};
      if (op.out2 >= 0) c.out2 = Desc{nullptr, p.body.slots[op.out2].len};
      if (n->storage == Node::Transient)
        transient_sites.push_back(static_cast<uint32_t>(site));
    }
  }

  void collect(const Node& n) {
    if (n.kind == Node::KernelCall) {
      if (n.site >= sites.size())
        throw std::logic_error("structured loop site numbering is stale");
      sites[n.site] = &n;
    }
    if (n.memo) {
      if (n.memo_index < 0 ||
          static_cast<size_t>(n.memo_index) >= memo_stride.size())
        throw std::logic_error("structured loop memo numbering is stale");
      memo_nodes[static_cast<size_t>(n.memo_index)] = &n;
      int64_t& stride = memo_stride[static_cast<size_t>(n.memo_index)];
      for (int slot : n.memo_outs) stride = add(stride, p.body.slots[slot].len);
    }
    if (n.trace && (n.trace_index < 0 ||
                    static_cast<size_t>(n.trace_index) >= trace_ordinal.size()))
      throw std::logic_error("structured loop trace numbering is stale");
    if (n.kind == Node::For && n.storage == Node::Transient)
      transient_loops.push_back(&n);
    for (const auto& c : n.children) collect(c);
  }

  void release() {
    arena.clear();
    versions.clear();
    owner.clear();
    handles.clear();
    undo.clear();
    records.clear();
    target_refs.clear();
    adjoint_size = 0;
    reverse_ready = false;
  }
};

struct Execution {
  const StructuredLoop& p;
  LoopState& s;
  KernelCtx& outer;
  enum Flow { Normal, Break, Continue };

  double* value(int slot) const {
    return s.versions[static_cast<size_t>(s.bindings[slot])].value;
  }
  double* adj(int64_t version) const {
    const int64_t a = s.versions[static_cast<size_t>(version)].adjoint;
    if (a >= 0) return s.adjoints.data() + a;
    if (a == -1) return nullptr;
    const auto& in = p.imports[static_cast<size_t>(-(a + 2))];
    double* base = outer.in_adj[in.input].data;
    return base ? base + in.offset : nullptr;
  }
  bool active(int64_t version) const {
    const int64_t a = s.versions[static_cast<size_t>(version)].adjoint;
    if (a >= 0) return true;
    if (a == -1) return false;
    return outer.in_adj[p.imports[static_cast<size_t>(-(a + 2))].input].data !=
           nullptr;
  }
  int64_t make_version(double* value, int64_t adjoint) {
    s.versions.push_back(Version{value, adjoint});
    s.owner.push_back(-1);
    return static_cast<int64_t>(s.versions.size()) - 1;
  }
  int64_t reserve_adjoint(int64_t len) {
    const int64_t at = s.adjoint_size;
    s.adjoint_size = add(s.adjoint_size, len);
    return at;
  }

  void bind_inputs(const Op& op, KernelCtx& c) const {
    for (int k = 0; k < op.n_in; ++k) c.in[k].data = value(op.in[k]);
  }

  void run_transient(const Node& n, const Op& op, KernelCtx& c) {
    bind_inputs(op, c);
    double* w = s.workspace.data() + n.workspace;
    c.out.data = w;
    w += c.out.len;
    if (op.out2 >= 0) {
      c.out2.data = w;
      w += c.out2.len;
    }
    c.scratch = w;
    n.forward(c);
    s.bindings[op.out] = s.node_version[n.site];
    if (op.out2 >= 0) s.bindings[op.out2] = s.node_version2[n.site];
  }

  void run_retained(const Node& n, const Op& op, KernelCtx& c) {
    int64_t handles = -1;
    if (n.active) {
      handles = static_cast<int64_t>(s.handles.size());
      for (int k = 0; k < op.n_in; ++k)
        s.handles.push_back(s.bindings[op.in[k]]);
    }
    bind_inputs(op, c);
    const int64_t out_len = c.out.len, out2_len = op.out2 >= 0 ? c.out2.len : 0;
    double* block =
        s.arena.allocate(add(add(out_len, out2_len), n.kernel_scratch));
    c.out.data = block;
    if (op.out2 >= 0) c.out2.data = block + out_len;
    c.scratch = block + out_len + out2_len;
    n.forward(c);
    const int64_t out =
        make_version(block, n.active ? reserve_adjoint(out_len) : -1);
    s.bindings[op.out] = out;
    int64_t out2 = -1;
    if (op.out2 >= 0) {
      out2 = make_version(block + out_len,
                          n.active ? reserve_adjoint(out2_len) : -1);
      s.bindings[op.out2] = out2;
    }
    if (n.active)
      s.records.push_back(
          Record{Record::Kernel, n.site, handles, out, out2, 0, c.scratch});
  }

  void run_in_place(const Node& n, const Op& op, KernelCtx& c) {
    const int base_slot = op.in[0];
    const auto& spec = *static_cast<const DynamicIndexSpec*>(op.udata);
    const IndexInputLayout layout = require_index_input_layout(spec, true);
    const int64_t len = p.body.slots[base_slot].len;
    const int64_t rhs = s.bindings[op.in[layout.rhs]];
    const bool rhs_active = active(rhs);
    int64_t base = s.bindings[base_slot];
    if (s.owner[static_cast<size_t>(base)] != base_slot) {
      double* copy = s.arena.allocate(len);
      std::copy_n(s.versions[static_cast<size_t>(base)].value, len, copy);
      const bool needs_adjoint = rhs_active || active(base);
      const int64_t fresh =
          make_version(copy, needs_adjoint ? reserve_adjoint(len) : -1);
      s.owner[static_cast<size_t>(fresh)] = base_slot;
      s.records.push_back(
          Record{Record::Copy, n.site, 0, base, fresh, len, nullptr});
      s.bindings[base_slot] = base = fresh;
    } else if (rhs_active &&
               s.versions[static_cast<size_t>(base)].adjoint == -1) {
      s.versions[static_cast<size_t>(base)].adjoint = reserve_adjoint(len);
    }
    bind_inputs(op, c);
    double* values = s.versions[static_cast<size_t>(base)].value;
    c.out.data = values;
    const IndexRuntime runtime = validate_index(spec, c, true);
    const double* source = c.in[layout.rhs].data;
    const int64_t undo = static_cast<int64_t>(s.undo.size());
    selected_positions(spec, c, runtime, [&](int64_t i, int64_t at) {
      s.undo.push_back(static_cast<double>(at));
      s.undo.push_back(values[at]);
      values[at] = source[i];
    });
    s.records.push_back(Record{Record::InPlace, n.site, undo, base, rhs,
                               runtime.selected, nullptr});
  }

  template <class T>
  T replay(const std::vector<std::vector<T>>& traces, int index) {
    const size_t t = static_cast<size_t>(index);
    const size_t k = static_cast<size_t>(s.trace_ordinal[t]++);
    if (k >= traces[t].size())
      throw std::logic_error("structured control trace mismatch");
    return traces[t][k];
  }

  Flow forward(const Node& n) {
    if (!n.memo) return run(n);
    if (n.memo_outs.empty()) return s.memo_ready ? Normal : run(n);
    const size_t m = static_cast<size_t>(n.memo_index);
    const int64_t k = s.memo_ordinal[m]++;
    std::vector<double>& tape = s.memo_tape[m];
    if (!s.memo_ready) {
      const Flow flow = run(n);
      for (int slot : n.memo_outs) {
        const double* v = value(slot);
        tape.insert(tape.end(), v, v + p.body.slots[slot].len);
      }
      ++s.memo_entries[m];
      return flow;
    }
    if (k >= s.memo_entries[m])
      throw std::logic_error("structured memo trace mismatch");
    double* v = tape.data() + k * s.memo_stride[m];
    size_t j = 0;
    for (; j < n.memo_fresh; ++j) {
      s.bindings[n.memo_outs[j]] = make_version(v, -1);
      v += p.body.slots[n.memo_outs[j]].len;
    }
    const int64_t* shared = s.memo_shared.data() + s.memo_shared_base[m];
    for (; j < n.memo_outs.size(); ++j) {
      const int64_t version = shared[j - n.memo_fresh];
      s.versions[static_cast<size_t>(version)].value = v;
      s.bindings[n.memo_outs[j]] = version;
      v += p.body.slots[n.memo_outs[j]].len;
    }
    ++s.memo_restores;
    return Normal;
  }

  Flow run(const Node& n) {
    switch (n.kind) {
      case Node::Sequence:
        for (const auto& child : n.children) {
          const Flow flow = forward(child);
          if (flow != Normal) return flow;
        }
        return Normal;
      case Node::KernelCall: {
        const Op& op = p.body.ops[n.op];
        KernelCtx& c = s.ctx[n.site];
        if (n.invariant_loop >= 0 &&
            s.node_generation[n.site] == s.loop_generation[n.invariant_loop]) {
          s.bindings[op.out] = s.node_version[n.site];
          if (op.out2 >= 0) s.bindings[op.out2] = s.node_version2[n.site];
          return Normal;
        }
        switch (n.storage) {
          case Node::Transient:
            run_transient(n, op, c);
            break;
          case Node::Retained:
            run_retained(n, op, c);
            break;
          case Node::InPlace:
            run_in_place(n, op, c);
            break;
        }
        if (n.invariant_loop >= 0) {
          s.node_generation[n.site] = s.loop_generation[n.invariant_loop];
          s.node_version[n.site] = s.bindings[op.out];
          if (op.out2 >= 0) s.node_version2[n.site] = s.bindings[op.out2];
        }
        return Normal;
      }
      case Node::Alias:
        s.bindings[n.dst] = s.bindings[n.src];
        s.owner[static_cast<size_t>(s.bindings[n.src])] = -1;
        return Normal;
      case Node::If: {
        size_t arm;
        if (n.trace && s.memo_ready) {
          arm = replay(s.trace_arms, n.trace_index);
        } else {
          arm = value(n.condition)[0] != 0.0 ? 0 : 1;
          if (n.trace)
            s.trace_arms[static_cast<size_t>(n.trace_index)].push_back(
                static_cast<uint8_t>(arm));
        }
        return forward(n.children[arm]);
      }
      case Node::For: {
        ++s.loop_generation[n.loop_index];
        const double lo = value(n.lower)[0], hi = value(n.upper)[0];
        if (!std::isfinite(lo) || !std::isfinite(hi) || std::trunc(lo) != lo ||
            std::trunc(hi) != hi || lo < std::numeric_limits<int32_t>::min() ||
            hi < std::numeric_limits<int32_t>::min() ||
            lo > std::numeric_limits<int32_t>::max() ||
            hi > std::numeric_limits<int32_t>::max())
          throw std::logic_error("structured loop invalid integer bounds");
        const int64_t count = hi >= lo ? static_cast<int64_t>(hi - lo) + 1 : 0;
        if (n.storage == Node::Transient) {
          const int64_t version = s.loop_version[n.loop_index];
          double* iterator = s.versions[static_cast<size_t>(version)].value;
          for (int64_t i = 0; i < count; ++i) {
            *iterator = lo + static_cast<double>(i);
            s.bindings[n.iterator] = version;
            if (forward(n.children[0]) == Break) break;
          }
          return Normal;
        }
        for (int64_t i = 0; i < count; ++i) {
          double* iterator = s.arena.allocate(1);
          *iterator = lo + static_cast<double>(i);
          s.bindings[n.iterator] = make_version(iterator, -1);
          if (forward(n.children[0]) == Break) break;
        }
        return Normal;
      }
      case Node::While: {
        ++s.loop_generation[n.loop_index];
        if (n.trace && s.memo_ready) {
          for (int64_t i = replay(s.trace_counts, n.trace_index); i-- > 0;)
            if (forward(n.children[1]) == Break) break;
          return Normal;
        }
        int64_t count = 0;
        for (;;) {
          if (forward(n.children[0]) == Break) break;
          if (value(n.condition)[0] == 0.0) break;
          ++count;
          if (forward(n.children[1]) == Break) break;
        }
        if (n.trace)
          s.trace_counts[static_cast<size_t>(n.trace_index)].push_back(count);
        return Normal;
      }
      case Node::Break:
        return Break;
      case Node::Continue:
        return Continue;
      case Node::Target:
        s.target_refs.push_back(s.bindings[n.src]);
        return Normal;
    }
    throw std::logic_error("invalid structured node");
  }

  void backward() {
    for (size_t i = s.records.size(); i-- > 0;) {
      const Record& r = s.records[i];
      switch (r.kind) {
        case Record::Kernel: {
          const Node& n = *s.sites[r.site];
          const Op& op = p.body.ops[n.op];
          KernelCtx& c = s.ctx[r.site];
          for (int k = 0; k < op.n_in; ++k) {
            const int64_t v = s.handles[static_cast<size_t>(r.handles + k)];
            c.in[k].data = s.versions[static_cast<size_t>(v)].value;
            c.in_adj[k].data = adj(v);
          }
          c.out.data = s.versions[static_cast<size_t>(r.out)].value;
          c.out_adj_vec.data = adj(r.out);
          if (c.out.len == 1) c.out_adj = c.out_adj_vec.data[0];
          if (r.out2 >= 0) {
            c.out2.data = s.versions[static_cast<size_t>(r.out2)].value;
            c.out2_adj = *adj(r.out2);
          }
          c.scratch = r.scratch;
          n.backward(c);
          break;
        }
        case Record::InPlace: {
          double* values = s.versions[static_cast<size_t>(r.out)].value;
          double* adj_base = adj(r.out);
          double* adj_rhs = adj(r.out2);
          for (int64_t k = r.count; k-- > 0;) {
            const size_t entry = static_cast<size_t>(r.handles + 2 * k);
            const int64_t at = static_cast<int64_t>(s.undo[entry]);
            if (adj_base) {
              if (adj_rhs) adj_rhs[k] += adj_base[at];
              adj_base[at] = 0;
            }
            values[at] = s.undo[entry + 1];
          }
          break;
        }
        case Record::Copy: {
          double* from = adj(r.out);
          double* to = adj(r.out2);
          if (from && to)
            for (int64_t k = 0; k < r.count; ++k) from[k] += to[k];
          break;
        }
      }
    }
  }
};

KernelState* make_loop_state(const Op& op, const Slot*) {
  return new LoopState(*static_cast<const StructuredLoop*>(op.udata));
}

LoopState& require_state(KernelCtx& ctx) {
  auto* state = static_cast<LoopState*>(ctx.state);
  if (!state || !ctx.udata || &state->p != ctx.udata)
    throw std::logic_error("structured loop has no bound executor state");
  return *state;
}
}  // namespace

void StructuredLoop::prepare() {
  initial_size = 0;
  for (auto& s : body.slots) {
    if (s.len < 0) throw std::invalid_argument("negative structured slot");
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
  node_count = site_count = loop_count = memo_count = trace_count = 0;
  workspace_size = 0;
  std::vector<char> out_seen(body.slots.size(), 0);
  prepare_node(*this, root, 0, 0, out_seen);
  classify(*this);
  body.compact_idata();
}

void structured_loop_forward(KernelCtx& ctx) {
  LoopState& s = require_state(ctx);
  const StructuredLoop& p = s.p;
  s.release();
  int64_t expected = p.has_target ? 1 : 0;
  for (int slot : p.outputs) expected += p.body.slots[slot].len;
  if (expected != ctx.out.len)
    throw std::logic_error("structured output size mismatch");
  Execution e{p, s, ctx};
  double* initial = s.arena.allocate(p.initial_size);
  std::fill_n(initial, p.initial_size, 0.0);
  s.bindings.resize(p.body.slots.size());
  for (size_t slot = 0; slot < p.body.slots.size(); ++slot)
    s.bindings[slot] = e.make_version(initial + p.body.slots[slot].offset, -1);
  for (const auto& fill : p.fills)
    std::copy(fill.second.begin(), fill.second.end(),
              initial + p.body.slots[fill.first].offset);
  for (size_t ordinal = 0; ordinal < p.imports.size(); ++ordinal) {
    const auto& in = p.imports[ordinal];
    const Slot& slot = p.body.slots[in.slot];
    if (in.input >= ctx.n_in || in.offset > ctx.in[in.input].len ||
        slot.len > ctx.in[in.input].len - in.offset)
      throw std::logic_error("structured import exceeds graph input");
    std::copy_n(ctx.in[in.input].data + in.offset, slot.len,
                initial + slot.offset);
    s.versions[static_cast<size_t>(s.bindings[in.slot])].adjoint =
        ctx.in_adj[in.input].data ? -(static_cast<int64_t>(ordinal) + 2) : -1;
  }
  for (uint32_t site : s.transient_sites) {
    const Node& n = *s.sites[site];
    const Op& op = p.body.ops[n.op];
    double* w = s.workspace.data() + n.workspace;
    s.node_version[site] = e.make_version(w, -1);
    if (op.out2 >= 0)
      s.node_version2[site] = e.make_version(w + p.body.slots[op.out].len, -1);
  }
  for (const Node* n : s.transient_loops)
    s.loop_version[n->loop_index] =
        e.make_version(s.workspace.data() + n->workspace, -1);
  std::fill(s.node_generation.begin(), s.node_generation.end(), -1);
  std::fill(s.loop_generation.begin(), s.loop_generation.end(), 0);
  std::fill(s.memo_ordinal.begin(), s.memo_ordinal.end(), 0);
  std::fill(s.trace_ordinal.begin(), s.trace_ordinal.end(), 0);
  if (!s.memo_ready) {
    for (auto& tape : s.memo_tape) tape.clear();
    for (auto& arms : s.trace_arms) arms.clear();
    for (auto& counts : s.trace_counts) counts.clear();
    std::fill(s.memo_entries.begin(), s.memo_entries.end(), 0);
  } else {
    for (auto& version : s.memo_shared) version = e.make_version(nullptr, -1);
  }
  const bool replaying = s.memo_ready;
  s.memo_restores = 0;
  for (auto& c : s.ctx) c.eval_state = ctx.eval_state;

  e.forward(p.root);
  int64_t pos = 0;
  for (int slot : p.outputs) {
    std::copy_n(e.value(slot), p.body.slots[slot].len, ctx.out.data + pos);
    pos += p.body.slots[slot].len;
  }
  if (p.has_target) {
    s.target_work.resize(s.target_refs.size());
    for (size_t i = 0; i < s.target_refs.size(); ++i)
      s.target_work[i] =
          s.versions[static_cast<size_t>(s.target_refs[i])].value[0];
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
  if (s.report_tape && (replaying || p.memo_count == 0)) {
    s.report_tape = false;
    size_t arena_used = 0;
    for (const auto& block : s.arena.blocks) arena_used += block.used;
    size_t kernel_records = 0, copies = 0, updates = 0, memo_tape = 0;
    for (const auto& r : s.records) {
      kernel_records += r.kind == Record::Kernel;
      copies += r.kind == Record::Copy;
      updates += r.kind == Record::InPlace;
    }
    for (const auto& tape : s.memo_tape) memo_tape += tape.size();
    std::fprintf(stderr,
                 "stanli_structured tape: arena=%zu adjoints=%lld versions=%zu "
                 "handles=%zu kernel_records=%zu updates=%zu undo=%zu "
                 "copies=%zu targets=%zu workspace=%zu memo_nodes=%zu "
                 "memo_restores=%zu memo_tape=%zu traces=%zu\n",
                 arena_used, static_cast<long long>(s.adjoint_size),
                 s.versions.size(), s.handles.size(), kernel_records, updates,
                 s.undo.size() / 2, copies, s.target_refs.size(),
                 s.workspace.size(), p.memo_count, s.memo_restores, memo_tape,
                 p.trace_count);
  }
  s.memo_ready = true;
  s.reverse_ready = true;
}

void structured_loop_backward(KernelCtx& ctx) {
  LoopState& s = require_state(ctx);
  const StructuredLoop& p = s.p;
  if (!s.reverse_ready)
    throw std::logic_error(
        "structured reverse has no successful forward state");
  s.reverse_ready = false;
  struct Release {
    LoopState& state;
    ~Release() { state.release(); }
  } release{s};
  s.adjoints.assign(static_cast<size_t>(s.adjoint_size), 0.0);
  Execution e{p, s, ctx};
  int64_t pos = 0;
  for (int slot : p.outputs) {
    const int64_t len = p.body.slots[slot].len;
    if (double* a = e.adj(s.bindings[slot]))
      for (int64_t i = 0; i < len; ++i) a[i] += ctx.out_adj_vec.data[pos + i];
    pos += len;
  }
  if (p.has_target)
    for (int64_t ref : s.target_refs)
      if (double* a = e.adj(ref)) *a += ctx.out_adj_vec.data[pos];
  e.backward();
}

void register_structured_loop_kernel() {
  register_kernel(OP_LOOP, {structured_loop_forward, structured_loop_backward,
                            nullptr, make_loop_state});
  register_kernel(OP_COMPARE, {compare_forward, nullptr, nullptr});
  register_kernel(OP_INT_ARITH, {int_forward, nullptr, nullptr});
  register_kernel(OP_INDEX_DYNAMIC, {index_forward, index_backward, nullptr});
  register_kernel(OP_SET_INDEX_DYNAMIC,
                  {set_index_forward, set_index_backward, set_index_scratch});
}
}  // namespace stanli
