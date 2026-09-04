#include <stanli/algebra.hpp>
#include <stanli/callable_transform.hpp>
#include <stanli/compile.hpp>
#include <stanli/unconstrain.hpp>
#include <stanli/constfold.hpp>
#include <stanli/cse.hpp>
#include <stanli/dae.hpp>
#include <stanli/higher_order_eval.hpp>
#include <stanli/expression_layout.hpp>
#include <stanli/inplace.hpp>
#include <stanli/mir_message.hpp>
#include <stanli/mir_prog.hpp>
#include <stanli/mir.hpp>
#include <stanli/mir_decode.hpp>
#include <stanli/mir_interp.hpp>
#include <stanli/ode.hpp>
#include <stanli/ode_adjoint.hpp>
#include <stanli/optable.hpp>
#include <stanli/island.hpp>
#include <stanli/structured_loop.hpp>
#include <stanli/partition.hpp>
#include <stanli/quadrature.hpp>
#include <stanli/reroll.hpp>
#include <stanli/regular_builtin.hpp>
#include <stanli/structured_check.hpp>
#include <stanli/wa_interp.hpp>

#include "reroll_profile.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <array>
#include <chrono>
#include <functional>
#include <initializer_list>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "lower_internal.hpp"

namespace stanli {
namespace lower_detail {

StructuredMode read_structured_mode() {
  const char* flag = std::getenv("STANLI_STRUCTURED_LOOPS");
  if (!flag || !*flag || std::string_view(flag) == "auto")
    return StructuredMode::Auto;
  if (std::string_view(flag) == "0") return StructuredMode::Off;
  if (std::string_view(flag) == "1") return StructuredMode::Prefer;
  if (std::string_view(flag) == "force") return StructuredMode::Force;
  // An unrecognized policy must preserve the established representation.
  return StructuredMode::Off;
}
// Static C++ scalar type without lowering/evaluating the expression. This
// is intentionally small: ordinary ops propagate Val::autodiff from their
// lowered operands; only a data-condition ternary needs the unchosen arm.
bool Lowering::expression_autodiff(const mir::Expr& e) const {
  if (e.unsized.leaf == mir::UnsizedLeaf::Int || e.data_only) return false;
  if (e.promoted) return scalar_autodiff();
  if (e.kind == mir::Expr::Var) {
    const auto formal = udf_formal_autodiff.find(e.name);
    if (formal != udf_formal_autodiff.end()) return formal->second;
    const auto value = scope.find(e.name);
    return value != scope.end() ? value->second.autodiff : scalar_autodiff();
  }
  if (e.kind == mir::Expr::TernaryIf && e.args.size() == 3)
    return expression_autodiff(e.args[1]) || expression_autodiff(e.args[2]);
  bool autodiff = false;
  for (const mir::Expr& arg : e.args)
    autodiff = autodiff || expression_autodiff(arg);
  return autodiff;
}
Lowering::Lowering(const DataMap& d, PrepTrace& p, const char* graph_name,
                   std::shared_ptr<ShapeInterner> pool)
    : data(d), shape_pool(std::move(pool)), prep(p), prep_graph(graph_name) {}
// Every index the lowering sees is a bind-time constant, so what CmdStan
// bounds-checks at runtime is checked here instead.
std::vector<int64_t> Lowering::index_positions(const mir::Expr& ix,
                                               int64_t extent, const char* what,
                                               const std::string& raw) {
  std::vector<int64_t> out;
  if (ix.name == "IndexAll") {
    for (int64_t i = 0; i < extent; ++i) out.push_back(i);
    return out;
  }
  if (ix.name == "IndexSingle") {
    const int64_t i = eval_int(ix.args[0]);
    check_index(i, extent, what, raw);
    return {i - 1};
  }
  if (is_range(ix)) {
    const StaticRange range = *static_range(ix, extent);
    check_range(range.lo, range.hi, extent, what, raw);
    for (int64_t i = range.lo; i <= range.hi; ++i) out.push_back(i - 1);
    return out;
  }
  if (ix.name == "IndexMulti") {
    DataMap::Entry iv = eval_pure(ix.args[0], "an index list");
    if (!iv.is_int) fail(std::string(what) + " needs int data", raw);
    for (int i : iv.i) {
      check_index(i, extent, what, raw);
      out.push_back(i - 1);
    }
    return out;
  }
  fail(std::string("unsupported ") + what + " " + ix.name, raw);
}
// Target models build int arrays in ascending contiguous writes.  Track the
// initialized prefix in O(1) per immutable slot: overwrites inside it are
// safe, an adjacent write extends it, and any gap/stride fails closed.  The
// interval hull may retain overwritten values, conservatively widening the
// later overflow proof.
void Lowering::propagate_int_update(const Val& out_v, const Val& base,
                                    const Val& rhs, int64_t start,
                                    int64_t stride) {
  // A write of an observed value into an observed base stays observed:
  // splice the element into a copy of the base's entry.
  if (const DataMap::Entry* be = observation(base)) {
    const DataMap::Entry* re = observation(rhs);
    const int64_t rl = g.slots[rhs.slot].len;
    if ((rl == 0 || re) && g.slots[out_v.slot].len == g.slots[base.slot].len) {
      DataMap::Entry en = *be;
      bool ok = true;
      for (int64_t k = 0; k < rl; ++k) {
        const int64_t at = start + k * stride;
        if (at < 0 || at >= (int64_t)en.r.size()) {
          ok = false;
          break;
        }
        const double v = k < (int64_t)re->r.size()
                             ? re->r[(size_t)k]
                             : static_cast<double>(re->i.at((size_t)k));
        en.r[(size_t)at] = v;
        if (!en.i.empty()) en.i[(size_t)at] = (int)v;
      }
      if (ok) observe(out_v, std::move(en));
    }
  }
  const auto base_prefix = int_initialized_prefix.find(base.slot);
  const auto rhs_prefix = int_initialized_prefix.find(rhs.slot);
  const int64_t rhs_len = g.slots[rhs.slot].len;
  if (rhs_len == 0 && base_prefix != int_initialized_prefix.end() &&
      g.slots[out_v.slot].len == g.slots[base.slot].len) {
    int_initialized_prefix[out_v.slot] = base_prefix->second;
    const auto base_range = int_ranges.find(base.slot);
    if (base_range == int_ranges.end())
      int_ranges.erase(out_v.slot);
    else
      int_ranges[out_v.slot] = base_range->second;
    return;
  }
  if (base_prefix == int_initialized_prefix.end() ||
      rhs_prefix == int_initialized_prefix.end() ||
      rhs_prefix->second != rhs_len || stride != 1 || start < 0 ||
      start > base_prefix->second || rhs_len < 0 ||
      start > g.slots[out_v.slot].len - rhs_len ||
      g.slots[out_v.slot].len != g.slots[base.slot].len) {
    int_ranges.erase(out_v.slot);
    int_initialized_prefix.erase(out_v.slot);
    return;
  }
  int_initialized_prefix[out_v.slot] =
      std::max(base_prefix->second, start + rhs_len);

  const auto rhs_range = int_ranges.find(rhs.slot);
  if (rhs_range == int_ranges.end()) {
    int_ranges.erase(out_v.slot);
    return;
  }
  IntRange range = rhs_range->second;
  if (base_prefix->second > 0) {
    const auto base_range = int_ranges.find(base.slot);
    if (base_range == int_ranges.end()) {
      int_ranges.erase(out_v.slot);
      return;
    }
    range.lo = std::min(range.lo, base_range->second.lo);
    range.hi = std::max(range.hi, base_range->second.hi);
  }
  int_ranges[out_v.slot] = range;
}
long Lowering::eval_int(const mir::Expr& e) {
  if (expr_effectful(e))
    fail("effectful expression cannot be used as a compile-time integer",
         e.raw);
  switch (e.kind) {
    case mir::Expr::LitInt:
      return e.lit_i;
    case mir::Expr::Var: {
      auto it = int_env.find(e.name);
      if (it != int_env.end()) return it->second;
      DataMap::Entry* en = td.find(e.name);
      if (en && en->is_int && en->i.size() == 1) return en->i[0];
      // A structured integer may live in a graph slot while its interval
      // proof has collapsed to one value (for example d = rows(mat)).  That
      // value is safe for fixed storage geometry even though it is computed
      // again when the retained body executes.
      if (region_current) {
        const auto value = scope.find(e.name);
        if (value != scope.end()) {
          const auto range = int_ranges.find(value->second.slot);
          if (range != int_ranges.end() && range->second.lo == range->second.hi)
            return range->second.lo;
        }
      }
      fail("size expression needs unknown int " + e.name);
    }
    case mir::Expr::Indexed: {
      // O1 can leave an empty Indexed wrapper around a fully composed
      // integer access, just as it does for real-valued expressions.
      if (e.args.size() == 1) return eval_int(e.args[0]);
      DataMap::Entry* en =
          e.args[0].kind == mir::Expr::Var ? td.find(e.args[0].name) : nullptr;
      if (en && en->is_int && e.args.size() == 2 &&
          e.args[1].name == "IndexSingle") {
        const long index = eval_int(e.args[1].args[0]);
        if (index < 1 || (size_t)index > en->i.size())
          fail("integer index " + std::to_string(index) +
                   " out of bounds for size " + std::to_string(en->i.size()),
               e.raw);
        return en->i[(size_t)index - 1];
      }
      if (en && en->is_int && e.args.size() == 3 &&
          e.args[1].name == "IndexSingle" && e.args[2].name == "IndexSingle" &&
          en->dims.size() == 2) {
        const long row = eval_int(e.args[1].args[0]);
        const long col = eval_int(e.args[2].args[0]);
        if (row < 1 || row > en->dims[0] || col < 1 || col > en->dims[1])
          fail("integer matrix index out of bounds", e.raw);
        return en->i[(size_t)((col - 1) * en->dims[0] + row - 1)];
      }
      // dims(x)[k] and friends: evaluate the base as a compile-time
      // sequence, then index it.
      {
        std::vector<int> vals = const_ints(e.args[0]);
        if (e.args.size() == 2 && e.args[1].name == "IndexSingle") {
          const long ix = eval_int(e.args[1].args[0]);
          if (ix >= 1 && (size_t)ix <= vals.size()) return vals[ix - 1];
        }
      }
      fail("unsupported int index expression", e.raw);
    }
    case mir::Expr::TernaryIf: {
      if (e.args.size() != 3)
        fail("malformed conditional size expression", e.raw);
      const bool condition = eval_int(e.args[0]) != 0;
      return eval_int(e.args[condition ? 1 : 2]);
    }
    case mir::Expr::EOr: {
      if (e.args.size() != 2) fail("malformed logical size expression", e.raw);
      return eval_int(e.args[0]) != 0 || eval_int(e.args[1]) != 0;
    }
    case mir::Expr::EAnd: {
      if (e.args.size() != 2) fail("malformed logical size expression", e.raw);
      return eval_int(e.args[0]) != 0 && eval_int(e.args[1]) != 0;
    }
    case mir::Expr::Promotion:
      if (e.args.size() != 1) fail("malformed promoted size expression", e.raw);
      return eval_int(e.args[0]);
    case mir::Expr::FunApp:
      if (e.name == "sum" && e.args.size() == 1) {
        long acc = 0;
        for (int v : const_ints(e.args[0])) acc += v;
        return acc;
      }
      if (e.name == "Plus__") return eval_int(e.args[0]) + eval_int(e.args[1]);
      if (e.name == "Minus__") return eval_int(e.args[0]) - eval_int(e.args[1]);
      if (e.name == "Times__") return eval_int(e.args[0]) * eval_int(e.args[1]);
      if ((e.name == "Equals__" || e.name == "NEquals__" ||
           e.name == "Greater__" || e.name == "Geq__" || e.name == "Less__" ||
           e.name == "Leq__") &&
          e.args.size() == 2) {
        const auto scalar = [&](const mir::Expr& arg) -> double {
          if (arg.type_ == "UInt") return (double)eval_int(arg);
          if (auto evaluated = try_eval_pure(arg)) {
            if (evaluated->r.size() == 1) return evaluated->r[0];
          }
          if (arg.kind == mir::Expr::Var) {
            const auto it = scope.find(arg.name);
            if (it != scope.end())
              if (const DataMap::Entry* en = observation(it->second))
                if (en->r.size() == 1) return en->r[0];
          }
          fail("comparison operand is not known data", arg.raw);
        };
        const double lhs = scalar(e.args[0]), rhs = scalar(e.args[1]);
        if (e.name == "Equals__") return lhs == rhs;
        if (e.name == "NEquals__") return lhs != rhs;
        if (e.name == "Greater__") return lhs > rhs;
        if (e.name == "Geq__") return lhs >= rhs;
        if (e.name == "Less__") return lhs < rhs;
        return lhs <= rhs;
      }
      // Shape queries on slot-bound values (e.g. rows(v) on an inlined
      // UDF's vector argument) answer from binding-owned metadata before
      // the interpreter, which cannot recover vector orientation.
      if ((e.name == "rows" || e.name == "cols" || e.name == "size" ||
           e.name == "num_elements" || e.name == "FnLength") &&
          e.args.size() == 1 && e.args[0].kind == mir::Expr::Var) {
        auto sit = scope.find(e.args[0].name);
        if (sit != scope.end()) {
          const SlotInfo& si = sit->second.si;
          const int64_t len = g.slots[sit->second.slot].len;
          if (is_array(si)) {
            const ArrayShape& sh = array_shape(si);
            if (e.name == "size" || e.name == "FnLength")
              return sh.dims.front();
            if (e.name == "num_elements") return len;
            fail(e.name + " is undefined for an array value", e.raw);
          }
          const LogicalDims dims = logical_dims(si, len, e.name);
          if (e.name == "rows") return dims.rows;
          if (e.name == "cols") return dims.cols;
          return len;
        }
        auto dl = decls.find(e.args[0].name);
        if (dl != decls.end()) {
          const DeclView& sh = dl->second;
          if (is_array(sh.si)) {
            const ArrayShape& arr = array_shape(sh.si);
            if (e.name == "size" || e.name == "FnLength")
              return arr.dims.front();
            if (e.name == "num_elements") return sh.len;
            fail(e.name + " is undefined for an array declaration", e.raw);
          }
          const LogicalDims dims = logical_dims(sh.si, sh.len, e.name);
          if (e.name == "rows") return dims.rows;
          if (e.name == "cols") return dims.cols;
          return sh.len;
        }
        // A name td knows but neither scope nor decls does: the scalar
        // `int` input. bind_data fills both tables from a declared shape
        // and a scalar int has none, so it falls past both -- the one
        // case, not the none this used to claim. The data_only branch
        // below does not catch it either, because a shape query in a
        // real-valued context is not data_only: `real p = size(n)` in
        // transformed parameters is AutoDiffable, so it reached the
        // failure instead and cost the census stanc3's
        // function-signatures/math/matrix/size.stan.
        //
        // Asking the interpreter is what the previous copy here should
        // have done all along. It answers rows/cols off the MIR type, so
        // the rank-1 orientation bug that copy carried cannot come back
        // through this route.
        if (td.find(e.args[0].name)) {
          try {
            return td.as_int(e);
          } catch (const CompileError&) {
          }
        }
      }
      // Shape query on a COMPUTED value: --O1 inlining substitutes call
      // arguments into the callee's size expressions, so `rows(beta)`
      // arrives as `rows(segment(beta, pos[i], m[i]))`. Lower the
      // argument and answer from its slot metadata; any op this emits
      // is one the body was about to emit anyway.
      if ((e.name == "rows" || e.name == "cols" || e.name == "size" ||
           e.name == "num_elements" || e.name == "FnLength") &&
          e.args.size() == 1 && e.args[0].kind != mir::Expr::Var) {
        CallArguments actuals(*this, e);
        const Val v = actuals.at(0).value();
        const int64_t len = g.slots[v.slot].len;
        if (is_array(v.si)) {
          const ArrayShape& sh = array_shape(v.si);
          if (e.name == "size" || e.name == "FnLength") return sh.dims.front();
          if (e.name == "num_elements") return len;
          fail(e.name + " is undefined for an array value", e.raw);
        }
        const LogicalDims dims = logical_dims(v.si, len, e.name);
        if (e.name == "rows") return dims.rows;
        if (e.name == "cols") return dims.cols;
        return len;
      }
      // Anything else data-only the td interpreter can evaluate (sum of an
      // int array in a size expression, etc.).
      if (e.data_only) {
        try {
          return td.as_int(e);
        } catch (const CompileError&) {
        }
      }
      fail("unsupported int size function " + e.name, e.raw);
    default:
      fail("unsupported size expression", e.raw);
  }
}
int64_t Lowering::sized_len(const mir::SizedType& t, int64_t* rows,
                            int64_t* cols) {
  if (t.base == "SInt" || t.base == "SReal") return 1;
  if (t.base == "SVector" || t.base == "SRowVector") {
    const int64_t n = eval_int(t.dims[0]);
    if (n < 0) fail("negative vector extent", t.raw);
    return n;
  }
  if (t.base == "SMatrix") {
    const int64_t r = eval_int(t.dims[0]), c = eval_int(t.dims[1]);
    if (r < 0 || c < 0) fail("negative matrix extent", t.raw);
    if (rows) *rows = r;
    if (cols) *cols = c;
    return checked_product({r, c}, "matrix shape");
  }
  if (t.base == "SArray") {
    return checked_product(sized_dims(t), "array shape");
  }
  fail("unsupported sized type " + t.base, t.raw);
}
SlotInfo Lowering::view_of(const mir::SizedType& t, bool param_free) {
  SlotInfo si;
  si.param_free = param_free;
  if (t.base == "SVector")
    si.kind = ViewKind::Vector;
  else if (t.base == "SRowVector")
    si.kind = ViewKind::RowVector;
  else if (t.base == "SMatrix") {
    si.kind = ViewKind::Matrix;
    si.rows = eval_int(t.dims[0]);
    si.cols = eval_int(t.dims[1]);
  } else if (t.base == "SArray") {
    const std::vector<int64_t> dims = sized_dims(t);
    si.kind = ViewKind::Array;
    si.shape = shape_pool->intern(dims, leaf_kind(t.elem_base));
  }
  return si;
}
SlotInfo Lowering::indexed_view(const SlotInfo& base, size_t n_single,
                                int64_t out_len, const std::string& out_type) {
  SlotInfo si = view_of(out_type);
  si.param_free = base.param_free;
  if (!is_array(base)) return si;
  const ArrayShape& a = array_shape(base);
  const size_t outer = a.dims.size() - (size_t)leaf_rank(a.leaf);
  if (n_single < outer) {
    std::vector<int64_t> suffix(a.dims.begin() + n_single, a.dims.end());
    return array_view(std::move(suffix), a.leaf, base.param_free);
  }
  if (n_single == outer) {
    if (a.leaf == ViewKind::Matrix) {
      const size_t n = a.dims.size();
      return matrix_view(a.dims[n - 2], a.dims[n - 1], base.param_free);
    }
    si.kind = a.leaf;
    si.shape = 0;
    return si;
  }
  (void)out_len;
  return si;
}
bool Lowering::same_view(const SlotInfo& a, int64_t alen, const SlotInfo& b,
                         int64_t blen) const {
  if (a.kind != b.kind) return false;
  switch (a.kind) {
    case ViewKind::Flat:
      return a.shape == 0 && b.shape == 0 && alen == 1 && blen == 1;
    case ViewKind::Vector:
    case ViewKind::RowVector:
      return alen == blen;
    case ViewKind::Matrix:
      return a.rows == b.rows && a.cols == b.cols && a.rows * a.cols == alen &&
             b.rows * b.cols == blen;
    case ViewKind::Array:
      return a.shape != 0 && a.shape == b.shape && alen == blen;
  }
  return false;
}
ExpressionLayout Lowering::elementwise_layout(std::initializer_list<Val> inputs,
                                              bool packet_supported) const {
  if (inputs.size() == 0) return ExpressionLayout::unknown();
  bool all_scalar = true;
  bool all_known = true;
  bool all_packet_access = true;
  for (const Val& input : inputs) all_scalar = all_scalar && is_scalar(input);
  for (const Val& input : inputs) {
    if (is_scalar(input)) continue;
    all_known = all_known && input.layout.known();
    all_packet_access = all_packet_access && input.layout.packet_access();
  }
  return expression_layout::elementwise(all_scalar, packet_supported, all_known,
                                        all_packet_access);
}
// The active scalar type is an independent reason for scalar traversal:
// Matrix<var> has no packet reducer even when its source layout is direct.
// Otherwise the source layout describes the Eigen evaluator that Stan Math
// reduced before graph materialization.
Lowering::ReductionGrouping Lowering::reduction_grouping(const Val& value,
                                                         bool active) const {
  if (active) return ReductionGrouping::Scalar;
  switch (value.layout.kind) {
    case ExpressionLayout::Kind::Unknown:
      return ReductionGrouping::Unknown;
    case ExpressionLayout::Kind::Scalar:
      return ReductionGrouping::Scalar;
    case ExpressionLayout::Kind::Packet:
      return ReductionGrouping::Packet;
    case ExpressionLayout::Kind::Direct:
      return value.layout.element_offset == 0 ? ReductionGrouping::Packet
                                              : ReductionGrouping::Phased;
  }
  return ReductionGrouping::Unknown;
}
// Reducing a slot is valid only when its logical view spans exactly the
// container the MIR overload named. The layout controls grouping; these
// checks only prevent a partial or padded slot from being mistaken for a
// complete vector, matrix, or one-dimensional array.
bool Lowering::extrema_storage(mir::ExtremaSurface surface, const Val& value) {
  const int64_t len = g.slots[value.slot].len;
  switch (surface) {
    case mir::ExtremaSurface::RealVector:
      return is_vector(value.si) || is_row_vector(value.si);
    case mir::ExtremaSurface::RealMatrix:
      return is_matrix(value.si) &&
             checked_product({value.si.rows, value.si.cols},
                             "min/max matrix shape") == len;
    case mir::ExtremaSurface::RealArray:
    case mir::ExtremaSurface::IntArray: {
      if (!is_array(value.si)) return false;
      const ArrayShape& shape = array_shape(value.si);
      return shape.leaf == ViewKind::Flat && shape.dims.size() == 1 &&
             shape.dims[0] == len;
    }
    default:
      return false;
  }
}
IntRange Lowering::prove_runtime_int_extrema(const mir::Expr& e,
                                             const Val& value, int64_t len) {
  if (!in_write_array)
    fail("runtime integer min/max is supported only in generated quantities",
         e.raw);
  // Stan Math raises for an empty integer container; the forward graph
  // kernel cannot reproduce that exception at execution time.
  if (len == 0)
    fail("min/max over an empty int array stays on WaInterp", e.raw);
  if (value.si.param_free)
    fail("min/max needs a runtime-produced int array", e.raw);
  const auto initialized = int_initialized_prefix.find(value.slot);
  if (initialized == int_initialized_prefix.end() || initialized->second != len)
    fail("min/max int array is not definitely initialized", e.raw);
  const auto known = int_ranges.find(value.slot);
  if (known == int_ranges.end())
    fail("min/max int array has unproved integral slot values", e.raw);
  return known->second;
}
Lowering::Val Lowering::lower_extrema_reduction(const mir::Expr& e,
                                                CallArguments& actuals,
                                                const mir::ExtremaCall& call) {
  actuals.require_arity(1);
  Val value = actuals.at(0).value();
  const int64_t len = g.slots[value.slot].len;
  if (!extrema_storage(call.surface, value) || len < 0)
    fail("min/max argument is not the whole declared container", e.raw);

  const bool int_array = call.surface == mir::ExtremaSurface::IntArray;
  const bool active = value.autodiff && !in_write_array;
  const ReductionGrouping grouping =
      int_array ? ReductionGrouping::Scalar : reduction_grouping(value, active);
  if (grouping == ReductionGrouping::Unknown)
    fail("min/max expression grouping is not native", e.raw);
  const bool scalar = grouping == ReductionGrouping::Scalar;
  const bool phased = grouping == ReductionGrouping::Phased;
  const IntRange range =
      int_array ? prove_runtime_int_extrema(e, value, len) : IntRange{};
  Val result =
      with_layout(emit_value(OP_EXTREMA_VEC, {value}, 1, view_of(e.type_),
                             reduction_phase_idata(value, grouping, "min/max")),
                  ExpressionLayout::scalar());
  if (in_write_array || int_array) result.autodiff = false;
  // Bit 0 selects max. Bits 1 and 2 are an exclusive grouping selector:
  // scalar coefficient order and phased packet order respectively.
  g.ops.back().variant =
      static_cast<uint8_t>((call.kind == mir::ExtremaKind::Max ? 1u : 0u) |
                           (scalar ? 2u : 0u) | (phased ? 4u : 0u));
  if (int_array) {
    result.si.param_free = false;
    set_int_range(result, range.lo, range.hi);
  }
  return result;
}
Lowering::Val Lowering::lower_extrema_pair(const mir::Expr& e,
                                           CallArguments& actuals,
                                           mir::ExtremaKind kind) {
  actuals.require_arity(2);
  Val x = actuals.at(0).value();
  Val y = actuals.at(1).value();
  if (!is_scalar(x) || !is_scalar(y))
    fail("min/max scalar overload needs two scalar int arguments", e.raw);
  const bool maximum = kind == mir::ExtremaKind::Max;
  Val result = with_layout(
      emit_value(maximum ? OP_FMAX : OP_FMIN, {x, y}, 1, view_of("UInt")),
      ExpressionLayout::scalar());
  result.autodiff = false;
  result.si.param_free = false;
  const std::optional<IntRange> a = int_operand_range(e.args[0], x);
  const std::optional<IntRange> b = int_operand_range(e.args[1], y);
  if (a && b) {
    set_int_range(result,
                  maximum ? std::max(a->lo, b->lo) : std::min(a->lo, b->lo),
                  maximum ? std::max(a->hi, b->hi) : std::min(a->hi, b->hi));
  } else {
    set_int_initialized(result);
  }
  return result;
}
Lowering::LogicalDims Lowering::logical_dims(const SlotInfo& si, int64_t len,
                                             const std::string& what) {
  if (si.kind == ViewKind::Flat) {
    if (si.shape != 0 || len != 1) fail(what + ": malformed scalar view");
    return {1, 1};
  }
  if (is_vector(si)) return {len, 1};
  if (is_row_vector(si)) return {1, len};
  if (is_matrix(si)) {
    if (checked_product({si.rows, si.cols}, what) != len)
      fail(what + ": malformed matrix view");
    return {si.rows, si.cols};
  }
  fail(what + ": array values do not have one rows/cols view");
}
Lowering::Val Lowering::lower_dims(const mir::Expr& e, CallArguments& actuals) {
  if (e.args.size() != 1) fail("dims arity", e.raw);
  const std::vector<int64_t> dims =
      logical_shape(actuals.at(0).value(), "dims");
  std::vector<double> vals(dims.begin(), dims.end());
  const int slot = add_slot((int64_t)vals.size(), false);
  out.fills.emplace_back(slot, vals);
  Val v{slot, false, array_view({(int64_t)dims.size()}, ViewKind::Flat, true)};
  DataMap::Entry en;
  en.is_int = true;
  en.r = std::move(vals);
  en.i.assign(dims.begin(), dims.end());
  observe(v, std::move(en));
  return v;
}
void Lowering::validate_view(const SlotInfo& si, int64_t len,
                             const std::string& what) {
  if (is_array(si) != (si.shape != 0))
    fail(what + ": array kind and shape id disagree");
  if (si.kind == ViewKind::Flat) {
    if (len != 1) fail(what + ": flat logical value is not a scalar");
    return;
  }
  if (is_matrix(si)) {
    if (checked_product({si.rows, si.cols}, what) != len)
      fail(what + ": matrix extents do not match storage length");
    return;
  }
  if (is_array(si)) {
    if (checked_product(array_shape(si).dims, what) != len)
      fail(what + ": array extents do not match storage length");
    return;
  }
}
void Lowering::sync_data_local(const std::string& name, const mir::Expr& rhs,
                               const Val& v) {
  if (!v.si.param_free) {
    td.env().erase(name);
    return;
  }
  if (const DataMap::Entry* en = observation(v)) {
    td.env()[name] = *en;
    return;
  }
  // Evaluate before erasing the old binding: `x = x + data_step` reads the
  // previous x, and data-only while loops depend on retaining that value for
  // their next condition.
  auto evaluated = try_eval_pure(rhs);
  td.env().erase(name);
  if (evaluated) {
    DataMap::Entry en = std::move(*evaluated);
    td.env()[name] = en;
    observe(v, std::move(en));
  }
}
void Lowering::observe_indexed_rhs(const mir::Expr& rhs, const Val& v) {
  if (observation(v) || !v.si.param_free) return;
  if (auto evaluated = try_eval_pure(rhs)) {
    observe(v, std::move(*evaluated));
    return;
  }
  if (rhs.type_ != "UInt" || g.slots[v.slot].len != 1) return;
  try {
    const long value = eval_int(rhs);
    DataMap::Entry en;
    en.is_int = true;
    en.i = {static_cast<int>(value)};
    en.r = {static_cast<double>(value)};
    observe(v, std::move(en));
  } catch (const CompileError&) {
    // Observation is an optimization. Runtime integer expressions remain
    // graph values and deliberately do not acquire a compile-time binding.
  }
}
// CmdStan's var_context validates every declared dimension against the
// supplied values before it reads one, and throws std::invalid_argument
// naming the variable and both shapes. Without the same check the short
// side is read past its end, and a host that tells bad data from a
// broken model by the exception type sees the wrong answer. Only the
// element count is compared: JSON carries a nested shape but stanc has
// already flattened the read, and a declaration whose extents multiply
// out to the supplied count is the shape the reader would produce.
void Lowering::validate_data_dims(const std::string& name,
                                  const mir::SizedType& t) {
  if (!data.has(name)) return;
  const DataMap::Entry& en = data.at(name);
  // A declared-int variable must arrive integer-typed, as CmdStan's
  // var_context requires (JSON 1.0 is not an int there either). Without
  // this the entry binds as typeless reals and the failure surfaces at
  // whatever consumer touches it first, e.g. the gather index guard.
  if ((t.base == "SInt" || (t.base == "SArray" && t.elem_base == "SInt")) &&
      !en.is_int)
    throw std::invalid_argument(
        "int variable contained non-int values; processing stage=data "
        "initialization; variable name=" +
        name + "; base type=int");
  const int64_t found = (int64_t)std::max(en.r.size(), en.i.size());
  const std::vector<int64_t> declared = sized_dims(t);
  int64_t want = 1;
  for (int64_t d : declared) {
    if (d < 0) fail("negative extent for data " + name, t.raw);
    want *= d;
  }
  if (want == found) return;
  const auto tuple = [](const std::vector<int64_t>& dims) {
    std::string s = "(";
    for (size_t k = 0; k < dims.size(); ++k) {
      if (k) s += ',';
      s += std::to_string(dims[k]);
    }
    return s + ")";
  };
  throw std::invalid_argument(
      "mismatch in dimension declared and found in context; processing "
      "stage=data initialization; variable name=" +
      name + "; position=0; dims declared=" + tuple(declared) +
      "; dims found=" +
      tuple(en.dims.empty() ? std::vector<int64_t>{found} : en.dims));
}
void Lowering::scan_rebuild(const mir::Stmt& s, RebuildShape& shape) {
  switch (s.kind) {
    case mir::Stmt::Block:
    case mir::Stmt::SList:
      for (const auto& k : s.body) scan_rebuild(k, shape);
      return;
    case mir::Stmt::For: {
      std::set<std::string> bounds_reads;
      data_reads(s.lower, bounds_reads);
      data_reads(s.upper, bounds_reads);
      if (!bounds_reads.empty()) shape.supported = false;
      for (const auto& k : s.body) scan_rebuild(k, shape);
      return;
    }
    case mir::Stmt::Decl: {
      shape.decls.insert(s.decl_id);
      if (!s.has_init) return;
      std::set<std::string> reads;
      data_reads(s.init, reads);
      shape.reads.insert(reads.begin(), reads.end());
      if (!reads.empty()) {
        ++shape.loaders;
        shape.loader_lhs = s.decl_id;
      }
      return;
    }
    case mir::Stmt::Assignment: {
      shape.writes.insert(s.lhs);
      std::set<std::string> reads;
      data_reads(s.rhs, reads);
      for (const auto& ix : s.lhs_idx) data_reads(ix, reads);
      shape.reads.insert(reads.begin(), reads.end());
      if (!reads.empty()) {
        if (!s.lhs_idx.empty()) shape.supported = false;
        ++shape.loaders;
        shape.loader_lhs = s.lhs;
      }
      return;
    }
    default:
      // A generated input rebuild has no effects, conditionals, target
      // writes, validation calls, or returns. New statement kinds fall back
      // to interpretation rather than guessing which children are safe.
      shape.supported = false;
      return;
  }
}
bool Lowering::canonical_input_rebuild(const mir::Stmt& s,
                                       const std::set<std::string>& inputs) {
  if (s.kind != mir::Stmt::Block && s.kind != mir::Stmt::SList) return false;
  RebuildShape shape;
  scan_rebuild(s, shape);
  if (!shape.supported || shape.loaders != 1 || shape.reads.size() != 1)
    return false;
  const std::string& input = *shape.reads.begin();
  if (!inputs.count(input) || shape.loader_lhs.empty() ||
      shape.loader_lhs == input || !shape.decls.count(shape.loader_lhs) ||
      !shape.writes.count(input))
    return false;
  const auto allowed = [&](const std::string& name) {
    return name == input || name == shape.loader_lhs || name == "pos__";
  };
  for (const auto& name : shape.decls)
    if (!allowed(name)) return false;
  for (const auto& name : shape.writes)
    if (!allowed(name)) return false;
  return true;
}
void Lowering::bind_data(const mir::Program& p) {
  std::set<std::string> input_names;
  bool all_inputs_bound = true;
  bool use_prebound = std::getenv("STANLI_NO_DATA_PRELOAD") == nullptr;
  for (const auto& [name, type] : p.input_vars) {
    input_names.insert(name);
    if (!data.has(name)) {
      all_inputs_bound = false;
      continue;
    }
    // DataMap does not have the Stan schema, so JSON values spelled with
    // integer tokens carry an int mirror even when the declaration is
    // real. Reconstruct the typed value directly, without copying an
    // irrelevant mirror for a large real matrix.
    const DataMap::Entry& src = data.at(name);
    if (!use_prebound) {
      td.env()[name] = src;
      continue;
    }
    DataMap::Entry dst;
    const bool want_int = type.base == "SInt" ||
                          (type.base == "SArray" && type.elem_base == "SInt");
    dst.is_int = want_int;
    dst.r = src.r;
    dst.dims = src.dims;
    if (want_int) {
      if (!src.i.empty()) {
        dst.i = src.i;
      } else if (!src.r.empty()) {
        // Preserve the interpreter's existing error/coercion behavior for
        // malformed data instead of silently truncating real values here.
        use_prebound = false;
      }
    }
    td.env()[name] = std::move(dst);
  }
  use_prebound = use_prebound && all_inputs_bound;
  if (use_prebound) {
    // The generated reconstruction allocated the MIR-declared shape and
    // copied exactly that many flat elements. Normalize to the same shape;
    // a malformed length falls back to that checked interpreter path.
    for (const auto& [name, type] : p.input_vars) {
      DataMap::Entry& dst = td.env().at(name);
      if (static_cast<int64_t>(dst.r.size()) != sized_len(type)) {
        use_prebound = false;
        break;
      }
      dst.dims.clear();
      if (type.base != "SInt" && type.base != "SReal")
        for (const auto& d : type.dims) dst.dims.push_back(eval_int(d));
    }
  }
  if (use_prebound) {
    // Skipping the generated declarations also skips MirInterp's normal
    // declaration-geometry bookkeeping. Preserve it explicitly so checks
    // on an empty outer array still see its trailing vector/matrix extents,
    // which JSON [] cannot represent.
    for (const auto& input : p.input_vars) {
      const std::string& name = input.first;
      td.set_declared_dims(name, td.env().at(name).dims);
    }
  }
  auto record = [&](const std::string& name, const mir::SizedType& type) {
    if (type.base == "SInt") return;
    DeclView sh;
    sh.len = sized_len(type);
    sh.si = view_of(type, true);
    decls[name] = sh;
  };
  for (const auto& [name, type] : p.input_vars) {
    record(name, type);
    validate_data_dims(name, type);
  }
  for (const auto& st : p.prepare_data) {
    if (st.kind == mir::Stmt::Decl) record(st.decl_id, st.decl_type);
    // stanc's prepare_data first rebuilds every input from a flat
    // FnReadData buffer. DataMap has already parsed that buffer into the
    // same typed, column-major representation above. Replaying the
    // canonical matrix reconstruction means one interpreted assignment
    // per element (47 million for nn_rbm1bJ100) and used to dominate model
    // preparation. FnReadData is compiler-internal and cannot occur in
    // source transformed-data code, so a top-level statement containing it
    // is input hydration, not user computation.
    if (use_prebound &&
        ((st.kind == mir::Stmt::Decl && input_names.count(st.decl_id)) ||
         direct_input_load(st, input_names) ||
         canonical_input_rebuild(st, input_names)))
      continue;
    td.exec(st);
  }
  for (auto& [name, e] : td.env()) {
    if (e.is_int && e.i.size() == 1 && e.dims.empty()) int_env[name] = e.i[0];
  }
  int_env_data = int_env;
}
// Lazily materialize an env value as a data slot when log_prob uses it.
int Lowering::env_slot(const std::string& name) {
  DataMap::Entry* en = td.find(name);
  // Empty entries are real: `array[0] real x_r` is how ODE models spell
  // "no data for the system", and it still has to become a (zero-length)
  // slot when passed around.
  if (!en) return -1;
  auto dl = decls.find(name);
  if (en->r.empty() && dl == decls.end()) return -1;
  SlotInfo si;
  si.param_free = true;
  if (dl != decls.end()) si = dl->second.si;
  si.param_free = true;
  const bool nested_matrix =
      is_array(si) && array_shape(si).leaf == ViewKind::Matrix;
  // DataMap is first-index-fast. Graph arrays are outer-major, with only
  // an innermost matrix kept column-major, so normalize at materialization.
  std::vector<double> vals = graph_order(*en, is_matrix(si), nested_matrix);
  validate_view(si, (int64_t)vals.size(), "data value " + name);
  const int s = add_slot((int64_t)vals.size(), false);
  out.fills.emplace_back(s, vals);
  Val v{s, false, si, owning_layout(si)};
  scope[name] = v;
  observe(v, *en);
  return s;
}
// Materialize a declared local that has not received its first value yet.
// Stan initializes real locals and containers to NaN (and integer arrays
// to INT_MIN).  Both ordinary expression lowering and a runtime region's
// live-in binder must see that same value: a name can be read inside a
// parameter-dependent branch without being assigned by the branch, so it
// will not appear in the region's live-out/assignment scan.
int Lowering::uninitialized_decl_slot(const std::string& name) {
  auto dl = decls.find(name);
  if (dl == decls.end()) return -1;
  if (dl->second.deferred_shape)
    fail("unsized local read before its first assignment: " + name);
  SlotInfo si = dl->second.si;
  si.param_free = true;
  Val value{add_slot(dl->second.len, false), dl->second.autodiff, si,
            owning_layout(si), dl->second.runtime_dims};
  const double initial =
      dl->second.int_array
          ? static_cast<double>(std::numeric_limits<int>::min())
          : std::numeric_limits<double>::quiet_NaN();
  out.fills.emplace_back(value.slot,
                         std::vector<double>(dl->second.len, initial));
  if (dl->second.int_array) set_uninitialized_int_array(value);
  observe_fill(value, dl->second.int_array, initial, dl->second.len);
  scope[name] = value;
  return value.slot;
}
// ---- expressions ----------------------------------------------------------
Lowering::Val Lowering::lower_expr(const mir::Expr& e) {
  std::optional<Val> structured;
  if (region_current)
    structured = region_expr(e);
  else if (runtime_int_expression(e))
    structured = lower_runtime_scalar(e);
  Val value = structured ? *structured : lower_expr_impl(e);
  if (e.promoted) {
    value.autodiff = expression_autodiff(e);
  } else if (e.kind == mir::Expr::Var) {
    const auto formal = udf_formal_autodiff.find(e.name);
    if (formal != udf_formal_autodiff.end()) value.autodiff = formal->second;
  } else if (e.kind == mir::Expr::TernaryIf && e.args.size() == 3) {
    // A known data condition chooses one implementation value, but C++ has
    // already promoted the expression. MIR records that promoted type, so
    // no arm may be evaluated merely to rediscover it.
    value.autodiff = expression_autodiff(e);
  }
  return value;
}
Lowering::Val Lowering::lower_expr_impl(const mir::Expr& e) {
  switch (e.kind) {
    case mir::Expr::Var: {
      auto it = scope.find(e.name);
      if (it == scope.end()) {
        auto ii = int_env.find(e.name);
        if (ii != int_env.end())
          return constant(static_cast<double>(ii->second));
        const int s = env_slot(e.name);
        if (s >= 0) return scope.at(e.name);
        // A declared local read before its first write: Materialize
        // the same uninitialized container the indexed-assignment path would.
        if (uninitialized_decl_slot(e.name) >= 0) return scope.at(e.name);
        fail("unknown variable " + e.name);
      }
      return it->second;
    }
    case mir::Expr::Indexed: {
      // O1 index composition can leave an empty outer Indexed node around
      // an already-indexed value. The outer node owns the final result
      // type: for M[idx, idx] passed to a UDF that reads x[i, j], the inner
      // single/single access still says UMatrix and this wrapper says UReal.
      // Collapse the wrapper and lower the composed access with that final
      // type instead of rejecting the stale intermediate matrix type.
      if (e.args.size() == 1 && e.args[0].kind == mir::Expr::Indexed) {
        mir::Expr composed = e.args[0];
        composed.type_ = e.type_;
        composed.unsized = e.unsized;
        composed.data_only = e.data_only;
        composed.promoted = e.promoted;
        composed.raw = e.raw;
        return lower_expr(composed);
      }
      // All-Single indices with compile-time values -> element read.
      Val base = lower_expr(e.args[0]);
      // O1 drops a full-span read's All indices, so `m[:, :]` arrives as an
      // Indexed node with none left.
      if (e.args.size() == 1) return base;
      if (e.args.size() == 2 && e.args[1].name == "IndexAll") return base;
      if (std::any_of(
              e.args.begin() + 1, e.args.end(),
              [&](const mir::Expr& ix) { return runtime_selector(ix); }))
        return region_index(base, {e.args.begin() + 1, e.args.end()}, e.type_,
                            e.unsized);
      if (in_write_array && e.args.size() == 2 &&
          e.args[1].name == "IndexSingle" &&
          runtime_int_value(e.args[1].args[0])) {
        const Val index = lower_expr(e.args[1].args[0]);
        if (!is_scalar(index)) fail("runtime index is not scalar", e.raw);
        int64_t count = 0, width = 0;
        if (is_array(base.si)) {
          const ArrayShape& shape = array_shape(base.si);
          const size_t outer =
              shape.dims.size() - (size_t)leaf_rank(shape.leaf);
          if (outer != 1 || shape.dims.empty() ||
              shape.leaf == ViewKind::Matrix)
            fail("runtime index needs one outer array dimension", e.raw);
          count = shape.dims.front();
          width = count == 0 ? 0 : g.slots[base.slot].len / count;
        } else if (is_vector(base.si) || is_row_vector(base.si)) {
          count = g.slots[base.slot].len;
          width = 1;
        } else {
          fail("runtime index needs a vector or flat outer array", e.raw);
        }
        if (count <= 0 || width <= 0 || g.slots[base.slot].len != count * width)
          fail("runtime index has an invalid base shape", e.raw);
        SlotInfo si = indexed_view(base.si, 1, width, e.type_);
        Val value =
            emit_value(OP_DYNAMIC_SLICE, {base, index}, width, si,
                       {checked_immediate(count, "runtime index extent")});
        value.si.param_free = false;
        value.layout = owning_layout(value.si);
        return value;
      }
      bool all_single = true;
      for (size_t k = 1; k < e.args.size(); ++k)
        if (e.args[k].name != "IndexSingle") all_single = false;
      const std::vector<int64_t>* bdims = nullptr;
      if (is_array(base.si)) bdims = &array_shape(base.si).dims;
      const size_t n_idx = e.args.size() - 1;
      // One index on a matrix selects rows. A range or gather is not a
      // contiguous slice in column-major storage, so spell its gather.
      if (e.args.size() == 2 && is_matrix(base.si) &&
          (is_range(e.args[1]) || e.args[1].name == "IndexMulti")) {
        std::vector<int> rows;
        if (auto range = static_range(e.args[1], base.si.rows)) {
          check_range(range->lo, range->hi, base.si.rows, "matrix row range",
                      e.raw);
          for (int64_t i = range->lo; i <= range->hi; ++i)
            rows.push_back((int)i - 1);
        } else {
          DataMap::Entry iv = eval_pure(e.args[1].args[0], "a gather index");
          if (!iv.is_int) fail("matrix row gather needs int data", e.raw);
          for (int i : iv.i) {
            if (i < 1 || i > base.si.rows)
              fail("matrix row gather out of bounds", e.raw);
            rows.push_back(i - 1);
          }
        }
        std::vector<int> gather;
        gather.reserve(rows.size() * (size_t)base.si.cols);
        for (int64_t j = 0; j < base.si.cols; ++j)
          for (int i : rows) gather.push_back((int)(j * base.si.rows) + i);
        SlotInfo si =
            matrix_view((int64_t)rows.size(), base.si.cols, base.si.param_free);
        return with_layout(
            emit_value(OP_GATHER, {base}, (int64_t)rows.size() * base.si.cols,
                       si, gather),
            ExpressionLayout::scalar());
      }
      // A range over the outermost array dimension is contiguous because
      // graph storage keeps each whole outer element together. Preserve
      // the complete suffix shape even when its storage width is zero.
      if (e.args.size() == 2 && is_array(base.si) && is_range(e.args[1])) {
        const ArrayShape& sh = array_shape(base.si);
        const StaticRange range = *static_range(e.args[1], sh.dims.front());
        const int64_t lo = range.lo;
        const int64_t hi = range.hi;
        // hi < lo is an empty slice whatever the endpoints (CmdStan's
        // rvalue checks bounds only when a range is nonempty), and the
        // bounds are data, so rejecting it would make compilation
        // data-dependent.
        if (hi >= lo && (lo < 1 || hi > sh.dims.front()))
          fail("array outer range out of bounds", e.raw);
        std::vector<int64_t> out_dims = sh.dims;
        out_dims[0] = hi >= lo ? hi - lo + 1 : 0;
        const std::vector<int64_t> suffix(sh.dims.begin() + 1, sh.dims.end());
        const int64_t width = checked_product(suffix, "array element");
        const int64_t len = checked_product(out_dims, "array range");
        const int64_t offset = hi >= lo ? (lo - 1) * width : 0;
        SlotInfo si = array_view(std::move(out_dims), sh.leaf);
        return with_layout(
            emit_value(OP_SLICE, {base}, len, si,
                       {checked_immediate(offset, "array range offset")}),
            owning_layout(si));
      }
      // Between subrange read on a 1-D value: v[a:b] is contiguous.
      // hi < lo is empty, not negative-length.
      if (e.args.size() == 2 && is_range(e.args[1])) {
        const StaticRange range =
            *static_range(e.args[1], g.slots[base.slot].len);
        const int64_t lo = range.lo;
        const int64_t hi = range.hi;
        check_range(lo, hi, g.slots[base.slot].len, "range", e.raw);
        const int64_t len = hi >= lo ? hi - lo + 1 : 0;
        const int64_t offset = len ? lo - 1 : 0;
        return with_layout(
            emit_value(OP_SLICE, {base}, len, view_of(e.type_),
                       {checked_immediate(offset, "range offset")}),
            contiguous_layout(base, offset, "range"));
      }
      // A data gather on a one-dimensional scalar array keeps an exact
      // one-dimensional Array view. More structural forms need strides
      // and therefore stay fail-loud rather than masquerading as vectors.
      if (e.args.size() == 2 && is_array(base.si) &&
          e.args[1].name == "IndexMulti") {
        const ArrayShape& sh = array_shape(base.si);
        if (sh.leaf != ViewKind::Flat || sh.dims.size() != 1)
          fail("unsupported index expression", e.raw);
        DataMap::Entry iv = eval_pure(e.args[1].args[0], "a gather index");
        if (!iv.is_int || iv.i.size() != iv.r.size())
          fail("gather index must be int data", e.raw);
        std::vector<int> idata;
        idata.reserve(iv.i.size());
        for (int x : iv.i) {
          if (x < 1 || x > sh.dims[0])
            fail("array gather out of bounds", e.raw);
          idata.push_back(x - 1);
        }
        SlotInfo si = array_view({(int64_t)idata.size()}, ViewKind::Flat);
        return with_layout(
            emit_value(OP_GATHER, {base}, (int64_t)idata.size(), si, idata),
            owning_layout(si));
      }
      // Gather by a data int array: v[idx].
      if (e.args.size() == 2 && e.args[1].name == "IndexMulti") {
        // An empty index is a legitimate data-dependent gather (a slice
        // whose computed length is zero); an int-flagged entry whose int
        // mirror disagrees with its values is not.
        DataMap::Entry iv = eval_pure(e.args[1].args[0], "a gather index");
        if (!iv.is_int || iv.i.size() != iv.r.size())
          fail("gather index must be int data", e.raw);
        std::vector<int> idata;
        idata.reserve(iv.i.size());
        for (int x : iv.i) {
          check_index(x, g.slots[base.slot].len, "gather index", e.raw);
          idata.push_back(x - 1);
        }
        return with_layout(emit_value(OP_GATHER, {base}, (int64_t)idata.size(),
                                      view_of(e.type_), idata),
                           ExpressionLayout::scalar());
      }
      // Matrix row/column slices use the explicit logical view; physical
      // storage remains column-major even when either extent is zero.
      if (e.args.size() == 3 && is_matrix(base.si) &&
          e.args[1].name == "IndexSingle" && e.args[2].name == "IndexAll") {
        const int64_t i = eval_int(e.args[1].args[0]);
        check_index(i, base.si.rows, "matrix row", e.raw);
        return with_layout(
            emit_value(OP_SLICE_STRIDED, {base}, base.si.cols, view_of(e.type_),
                       {checked_immediate(i - 1, "matrix row offset"),
                        checked_immediate(base.si.rows, "matrix row stride")}),
            ExpressionLayout::scalar());
      }
      if (e.args.size() == 3 && is_matrix(base.si) &&
          e.args[1].name == "IndexAll" && e.args[2].name == "IndexSingle") {
        const int64_t j = eval_int(e.args[2].args[0]);
        check_index(j, base.si.cols, "matrix column", e.raw);
        const int64_t offset = (j - 1) * base.si.rows;
        return with_layout(
            emit_value(OP_SLICE, {base}, base.si.rows, view_of(e.type_),
                       {checked_immediate(offset, "matrix column offset")}),
            contiguous_layout(base, offset, "matrix column"));
      }
      // Column of a canonical graph-order 2-D array (array[N, S] real):
      // each outer element is contiguous, so successive rows sit S apart.
      if (e.args.size() == 3 && is_array(base.si) && bdims &&
          array_shape(base.si).leaf == ViewKind::Flat && bdims->size() == 2 &&
          e.args[1].name == "IndexAll" && e.args[2].name == "IndexSingle") {
        const int64_t k = eval_int(e.args[2].args[0]) - 1;
        const int64_t N = (*bdims)[0], S = (*bdims)[1];
        if (k < 0 || k >= S) fail("array column out of bounds", e.raw);
        return with_layout(
            emit_value(OP_SLICE_STRIDED, {base}, N,
                       array_view({N}, ViewKind::Flat),
                       {checked_immediate(k, "array column offset"),
                        checked_immediate(S, "array column stride")}),
            ExpressionLayout::scalar());
      }
      // Row range of the same layout: A[i, lo:hi] is contiguous.
      if (e.args.size() == 3 && is_array(base.si) && bdims &&
          (array_shape(base.si).leaf == ViewKind::Flat ||
           array_shape(base.si).leaf == ViewKind::Vector ||
           array_shape(base.si).leaf == ViewKind::RowVector) &&
          bdims->size() == 2 && e.args[1].name == "IndexSingle" &&
          is_range(e.args[2])) {
        const int64_t i = eval_int(e.args[1].args[0]);
        const int64_t S = (*bdims)[1];
        const StaticRange range = *static_range(e.args[2], S);
        const int64_t lo = range.lo;
        const int64_t hi = range.hi;
        check_index(i, (*bdims)[0], "array index", e.raw);
        check_range(lo, hi, S, "array range", e.raw);
        const int64_t len = hi >= lo ? hi - lo + 1 : 0;
        SlotInfo si = array_shape(base.si).leaf == ViewKind::Flat
                          ? array_view({len}, ViewKind::Flat)
                          : view_of(e.type_);
        const int64_t offset = len ? (i - 1) * S + lo - 1 : 0;
        const ExpressionLayout layout =
            array_shape(base.si).leaf == ViewKind::Flat
                ? owning_layout(si)
                : ExpressionLayout::direct(len ? lo - 1 : 0);
        return with_layout(
            emit_value(OP_SLICE, {base}, len, si,
                       {checked_immediate(offset, "array row range offset")}),
            layout);
      }
      // A whole vector leaf selected from array[N] vector[S]. The explicit
      // trailing All survives O1 for this spelling and addresses the same
      // contiguous outer-element block as the range directly above.
      if (e.args.size() == 3 && is_array(base.si) && bdims &&
          (array_shape(base.si).leaf == ViewKind::Vector ||
           array_shape(base.si).leaf == ViewKind::RowVector) &&
          bdims->size() == 2 && e.args[1].name == "IndexSingle" &&
          e.args[2].name == "IndexAll") {
        const int64_t i = eval_int(e.args[1].args[0]);
        const int64_t count = (*bdims)[0], width = (*bdims)[1];
        check_index(i, count, "array index", e.raw);
        const int64_t offset = (i - 1) * width;
        SlotInfo si = view_of(e.type_);
        return with_layout(
            emit_value(OP_SLICE, {base}, width, si,
                       {checked_immediate(offset, "array vector offset")}),
            owning_layout(si));
      }
      // Row-range column read M[a:b, j] (contiguous within the column).
      if (e.args.size() == 3 && is_matrix(base.si) && is_range(e.args[1]) &&
          e.args[2].name == "IndexSingle") {
        const StaticRange range = *static_range(e.args[1], base.si.rows);
        const int64_t lo = range.lo;
        const int64_t hi = range.hi;
        const int64_t j = eval_int(e.args[2].args[0]);
        check_index(j, base.si.cols, "matrix column", e.raw);
        check_range(lo, hi, base.si.rows, "matrix row range", e.raw);
        const int64_t len = hi >= lo ? hi - lo + 1 : 0;
        const int64_t offset = len ? (j - 1) * base.si.rows + lo - 1 : 0;
        return with_layout(
            emit_value(OP_SLICE, {base}, len, view_of(e.type_),
                       {checked_immediate(offset, "matrix row range offset")}),
            contiguous_layout(base, offset, "matrix row range"));
      }
      // Any two-axis matrix selection the slices above leave is the
      // Cartesian selection M[rows, cols], not a pairwise zip. Preserve
      // index-array order and duplicates; column-major output means
      // selected columns are outer and selected rows are inner in the flat
      // gather list.
      const auto is_matrix_selector = [](const mir::Expr& index) {
        return index.name == "IndexAll" || index.name == "IndexSingle" ||
               is_range(index) || index.name == "IndexMulti";
      };
      if (e.args.size() == 3 && is_matrix(base.si) &&
          is_matrix_selector(e.args[1]) && is_matrix_selector(e.args[2]) &&
          (e.args[1].name != "IndexSingle" ||
           e.args[2].name != "IndexSingle")) {
        const std::vector<int64_t> rows = index_positions(
            e.args[1], base.si.rows, "matrix row gather", e.raw);
        const std::vector<int64_t> cols = index_positions(
            e.args[2], base.si.cols, "matrix column gather", e.raw);
        std::vector<int> gather;
        gather.reserve(rows.size() * cols.size());
        for (int64_t j : cols)
          for (int64_t i : rows)
            gather.push_back(checked_immediate(j * base.si.rows + i,
                                               "matrix gather offset"));
        SlotInfo si = view_of(e.type_);
        si.param_free = base.si.param_free;
        if (e.type_ == "UMatrix")
          si = matrix_view((int64_t)rows.size(), (int64_t)cols.size(),
                           base.si.param_free);
        return with_layout(
            emit_value(OP_GATHER, {base},
                       (int64_t)rows.size() * (int64_t)cols.size(), si, gather),
            ExpressionLayout::scalar());
      }
      // Params/locals with recorded dims, laid out by flat_addr above.
      // Matrix views are col-major and never take this array-major path.
      if (all_single && bdims && n_idx <= bdims->size() &&
          !is_matrix(base.si)) {
        const auto& D = *bdims;
        const bool mat = array_shape(base.si).leaf == ViewKind::Matrix;
        std::vector<int64_t> ix;
        for (size_t d = 0; d < n_idx; ++d) {
          const int64_t one = eval_int(e.args[1 + d].args[0]);
          check_index(one, D[d], "array index", e.raw);
          ix.push_back(one - 1);
        }
        const Addr a = flat_addr(D, mat, ix);
        if (a.stride != 1)
          return with_layout(
              emit_value(OP_SLICE_STRIDED, {base}, a.len,
                         indexed_view(base.si, n_idx, a.len, e.type_),
                         {checked_immediate(a.off, "indexed offset"),
                          checked_immediate(a.stride, "indexed stride")}),
              ExpressionLayout::scalar());
        if (a.len == 1)
          return with_layout(
              emit_value(OP_INDEX, {base}, 1,
                         indexed_view(base.si, n_idx, 1, e.type_),
                         {checked_immediate(a.off, "indexed offset")}),
              ExpressionLayout::scalar());
        // One whole matrix out of the array keeps its shape, so a later
        // index on it can take the column-major paths above.
        SlotInfo si = indexed_view(base.si, n_idx, a.len, e.type_);
        return with_layout(
            emit_value(OP_SLICE, {base}, a.len, si,
                       {checked_immediate(a.off, "indexed offset")}),
            owning_layout(si));
      }
      // A full array-index prefix pins one vector/row_vector leaf element;
      // exactly one trailing range/all index then reads inside that leaf.
      // The prefix is not all-single-index in stanc's own sense (the trailing
      // index is a range), so this falls outside the block above even
      // though every array position is fixed. Graph storage keeps the
      // pinned leaf contiguous, so this is one contiguous read from its
      // start once flat_addr locates it.
      if (bdims && (array_shape(base.si).leaf == ViewKind::Vector ||
                    array_shape(base.si).leaf == ViewKind::RowVector)) {
        const size_t n_arr = bdims->size() - 1;
        const mir::Expr& last = e.args.back();
        bool prefix_single = e.args.size() == n_arr + 2 &&
                             (is_range(last) || last.name == "IndexAll");
        for (size_t d = 0; prefix_single && d < n_arr; ++d)
          if (e.args[1 + d].name != "IndexSingle") prefix_single = false;
        if (prefix_single) {
          std::vector<int64_t> ix;
          ix.reserve(n_arr);
          for (size_t d = 0; d < n_arr; ++d) {
            const int64_t one = eval_int(e.args[1 + d].args[0]);
            check_index(one, (*bdims)[d], "array index", e.raw);
            ix.push_back(one - 1);
          }
          const Addr a = flat_addr(*bdims, false, ix);
          int64_t lo = 1, hi = a.len;
          const bool ranged = is_range(last);
          if (ranged) {
            const StaticRange range = *static_range(last, a.len);
            lo = range.lo;
            hi = range.hi;
            check_range(lo, hi, a.len, "array leaf range", e.raw);
          }
          const int64_t len = hi >= lo ? hi - lo + 1 : 0;
          SlotInfo si = view_of(e.type_);
          const int64_t offset = len ? a.off + lo - 1 : a.off;
          const ExpressionLayout layout =
              ranged ? ExpressionLayout::direct(len ? lo - 1 : 0)
                     : owning_layout(si);
          return with_layout(
              emit_value(
                  OP_SLICE, {base}, len, si,
                  {checked_immediate(offset, "array vector slice offset")}),
              layout);
        }
      }
      // A single outer-array range kept in full, with fixed row/column
      // indices into every element's matrix: array[N] matrix[R, C][:, i,
      // j]. Graph storage keeps each matrix contiguous and array-major, so
      // this is a strided read of one scalar out of every element.
      if (e.args.size() == 4 && is_array(base.si) && bdims &&
          bdims->size() == 3 && array_shape(base.si).leaf == ViewKind::Matrix &&
          e.args[1].name == "IndexAll" && e.args[2].name == "IndexSingle" &&
          e.args[3].name == "IndexSingle") {
        const int64_t N = (*bdims)[0], R = (*bdims)[1], C = (*bdims)[2];
        const int64_t ri = eval_int(e.args[2].args[0]);
        const int64_t cj = eval_int(e.args[3].args[0]);
        check_index(ri, R, "matrix row", e.raw);
        check_index(cj, C, "matrix column", e.raw);
        const int64_t off = (cj - 1) * R + (ri - 1);
        SlotInfo si = array_view({N}, ViewKind::Flat, base.si.param_free);
        return with_layout(
            emit_value(OP_SLICE_STRIDED, {base}, N, si,
                       {checked_immediate(off, "matrix array cell offset"),
                        checked_immediate(R * C, "matrix array cell stride")}),
            owning_layout(si));
      }
      // Row of a column-major data matrix / 2-D array: strided slice.
      if (all_single && e.args.size() == 2 && is_matrix(base.si) &&
          e.type_ != "UReal" && e.type_ != "UInt") {
        const int64_t t = eval_int(e.args[1].args[0]);
        check_index(t, base.si.rows, "matrix row", e.raw);
        return with_layout(
            emit_value(OP_SLICE_STRIDED, {base}, base.si.cols, view_of(e.type_),
                       {checked_immediate(t - 1, "matrix row offset"),
                        checked_immediate(base.si.rows, "matrix row stride")}),
            ExpressionLayout::scalar());
      }
      // Data-only slicing with no native path (e.g. one matrix out of a
      // data array of matrices) evaluates at compile time.
      const bool base_seen = e.args[0].kind != mir::Expr::Var ||
                             td.find(e.args[0].name) != nullptr;
      if (base_seen) {
        if (auto v = fold_const(e)) return *v;
      }
      int64_t flat = 0;
      if (all_single && e.args.size() == 2 &&
          (e.type_ == "UReal" || e.type_ == "UInt")) {
        const int64_t one = eval_int(e.args[1].args[0]);
        check_index(one, g.slots[base.slot].len, "element", e.raw);
        flat = one - 1;
      } else if (all_single && e.args.size() == 3 && is_matrix(base.si) &&
                 (e.type_ == "UReal" || e.type_ == "UInt")) {
        const int64_t ri = eval_int(e.args[1].args[0]);
        const int64_t cj = eval_int(e.args[2].args[0]);
        check_index(ri, base.si.rows, "matrix row", e.raw);
        check_index(cj, base.si.cols, "matrix column", e.raw);
        flat = (cj - 1) * base.si.rows + (ri - 1);
      } else {
        std::string desc =
            "unsupported index expression: base=" +
            (e.args[0].kind == mir::Expr::Var ? e.args[0].name
                                              : std::string("<expr>"));
        for (size_t k = 1; k < e.args.size(); ++k)
          desc += " [" + (e.args[k].name.empty() ? "?" : e.args[k].name) + "]";
        desc += " type=" + e.type_;
        fail(desc, e.raw);
      }
      return with_layout(emit_value(OP_INDEX, {base}, 1, view_of(e.type_),
                                    {checked_immediate(flat, "index offset")}),
                         ExpressionLayout::scalar());
    }
    case mir::Expr::LitInt: {
      Val v = constant(static_cast<double>(e.lit_i));
      set_int_range(v, e.lit_i, e.lit_i);
      return v;
    }
    case mir::Expr::LitReal:
      return constant(e.lit);
    case mir::Expr::FunApp:
      return lower_funapp(e);
    case mir::Expr::TernaryIf: {
      if (expr_effectful(e.args[0]))
        fail("effectful expression cannot be a compile-time condition", e.raw);
      // Shape specialization and ordinary data evaluation can decide a
      // condition even when the complete expression's MIR adlevel is not
      // DataOnly (for example `rows(x) == 0 || theta > 0`).  Only the
      // genuinely unresolved case needs runtime control.
      if (auto condition = try_eval_pure(e.args[0]))
        return lower_expr(e.args[condition->r.at(0) != 0.0 ? 1 : 2]);
      return lower_runtime_ternary(e);
    }
    case mir::Expr::EOr:
    case mir::Expr::EAnd: {
      if (auto v = fold_const(e)) return *v;
      if (runtime_only(e)) return lower_runtime_ternary(e);
      fail("boolean operator on parameters unsupported", e.raw);
    }
    default: {
      if (auto v = fold_const(e)) return *v;
      fail("unsupported expression", e.raw.empty() ? e.name : e.raw);
    }
  }
}
bool Lowering::expr_has_jacobian(const mir::Expr& e) {
  if (e.kind == mir::Expr::FunApp) {
    CallableTransformSpec transform;
    if (callable_transform(e.name, &transform) &&
        transform.direction == TransformDirection::Jacobian)
      return true;
    // Stan permits Jacobian adjustments in a UDF precisely when its name
    // has this suffix. Conservatively carry a target through such a call;
    // an unused zero is cheaper than dropping a nested adjustment.
    if (e.fn_lib == mir::Expr::Lib::UserDefined &&
        transform_suffix(e.name, "_jacobian"))
      return true;
  }
  for (const auto& a : e.args)
    if (expr_has_jacobian(a)) return true;
  return false;
}
// Does `s` increment the target, explicitly or through a Jacobian call?
bool Lowering::has_target_pe(const mir::Stmt& s) {
  if (s.kind == mir::Stmt::TargetPE) return true;
  if ((s.has_init && expr_has_jacobian(s.init)) || expr_has_jacobian(s.rhs) ||
      expr_has_jacobian(s.target) || expr_has_jacobian(s.lower) ||
      expr_has_jacobian(s.upper) || expr_has_jacobian(s.cond))
    return true;
  for (const auto& e : s.fn_args)
    if (expr_has_jacobian(e)) return true;
  for (const auto& e : s.lhs_idx)
    if (expr_has_jacobian(e)) return true;
  for (const auto& k : s.body)
    if (has_target_pe(k)) return true;
  return false;
}
bool Lowering::needs_runtime_control(const mir::Stmt& s) {
  // A structured while owns every runtime decision in its body.  Promoting
  // its enclosing block would absorb UDF-local declarations and returns,
  // which are not live-outs of that outer region.
  if (s.kind == mir::Stmt::While) return false;
  if (s.kind == mir::Stmt::Block || s.kind == mir::Stmt::SList) {
    // This scan runs before the block is lowered, but loop bounds later in
    // the block can depend on scalar-int locals established by earlier
    // statements.  Mirror just that compile-time environment in statement
    // order.  In particular, stanc spells `int d = rows(x)` as a default
    // declaration followed by an assignment, and UDFs commonly use d to
    // size locals and loops.  Looking through the whole block without this
    // lexical state rejects an otherwise static write-array UDF.
    const auto saved = int_env;
    std::set<std::string> local_ints;
    bool found = false;
    try {
      for (const auto& child : s.body) {
        if (needs_runtime_control(child)) {
          found = true;
          break;
        }
        if (child.kind == mir::Stmt::Decl && child.decl_type.base == "SInt") {
          local_ints.insert(child.decl_id);
          int_env.erase(child.decl_id);
          if (child.has_init) int_env[child.decl_id] = eval_int(child.init);
        } else if (child.kind == mir::Stmt::Assignment &&
                   child.lhs_idx.empty() && local_ints.count(child.lhs)) {
          int_env[child.lhs] = eval_int(child.rhs);
        }
      }
    } catch (...) {
      int_env = saved;
      throw;
    }
    int_env = saved;
    return found;
  }
  if (s.kind == mir::Stmt::IfElse) {
    // This is a speculative write_array scan, so follow an already-known
    // arm exactly as ordinary lowering will.  Besides avoiding needless
    // work, this preserves Stan's reachability semantics for invalid shape
    // selectors in a dead statement arm.
    if (auto evaluated = try_eval_pure(s.cond)) {
      const size_t arm = evaluated->r.at(0) != 0.0 ? 0 : 1;
      return arm < s.body.size() && needs_runtime_control(s.body[arm]);
    }
    if (s.cond.data_only) return true;
  }
  if (s.kind == mir::Stmt::For) {
    const long lo = eval_int(s.lower), hi = eval_int(s.upper);
    if (lo > hi) return false;
    const auto old = int_env.find(s.loopvar);
    const bool had_old = old != int_env.end();
    const long old_value = had_old ? old->second : 0;
    bool found = false;
    // Scan under the same compile-time loop bindings ordinary lowering
    // will use. This keeps static conditions such as `if (t < N)` out of
    // a region without overlooking an arm that exists only at a later t.
    for (long v = lo; v <= hi && !found; ++v) {
      int_env[s.loopvar] = v;
      for (const auto& k : s.body)
        if (needs_runtime_control(k)) {
          found = true;
          break;
        }
    }
    if (had_old)
      int_env[s.loopvar] = old_value;
    else
      int_env.erase(s.loopvar);
    return found;
  }
  for (const auto& k : s.body)
    if (needs_runtime_control(k)) return true;
  return false;
}
// A Break/Continue selected by a runtime condition cannot be lowered as a
// standalone conditional island: its jump target belongs to the enclosing
// loop. Promote that whole loop to the necessity island instead. Nested
// loops own their own control statements and therefore stop this search.
bool Lowering::runtime_loop_control(const mir::Stmt& s, bool runtime_path) {
  if (s.kind == mir::Stmt::Break || s.kind == mir::Stmt::Continue)
    return runtime_path;
  if (s.kind == mir::Stmt::For || s.kind == mir::Stmt::While) return false;
  if (s.kind == mir::Stmt::IfElse) {
    if (auto evaluated = try_eval_pure(s.cond)) {
      const bool take_then = evaluated->r.at(0) != 0.0;
      if (take_then && !s.body.empty())
        return runtime_loop_control(s.body[0], runtime_path);
      if (!take_then && s.body.size() > 1)
        return runtime_loop_control(s.body[1], runtime_path);
      return false;
    }
    for (const auto& arm : s.body)
      if (runtime_loop_control(arm, true)) return true;
    return false;
  }
  for (const auto& child : s.body)
    if (runtime_loop_control(child, runtime_path)) return true;
  return false;
}
// Remove a return at the lexical end of a statement arm, preserving every
// statement that precedes it.  This is the structured form used by UDFs
// such as ctsem's mcalc: each arm returns, but one arm first updates a local
// matrix.  The updates can lower as an ordinary statement island and the
// two returned expressions can then join through a ternary value island.
bool Lowering::peel_terminal_return(mir::Stmt* s, mir::Expr* value) {
  if (s->kind == mir::Stmt::Return) {
    if (!s->has_init) return false;
    *value = s->rhs;
    s->kind = mir::Stmt::Skip;
    s->body.clear();
    return true;
  }
  if ((s->kind == mir::Stmt::Block || s->kind == mir::Stmt::SList) &&
      !s->body.empty())
    return peel_terminal_return(&s->body.back(), value);
  return false;
}
// Compile `s` (a statement region) or `e` (a ternary) into a program.
void Lowering::lower_island(const mir::Stmt* s, const mir::Expr* e,
                            IslandRegion* reg, Range* expr_out,
                            std::shared_ptr<IslandProg>* prog_out) {
  auto prog = std::make_shared<IslandProg>();
  ProgramCompiler c{*prog, fun_defs};
  c.in_write_array = in_write_array;
  // Non-returning statement calls may print or reject. A register program
  // would replay them during reverse mode, so ProgramCompiler refuses them
  // until necessity islands have an execute-once effect path.
  for (const auto& [name, v] : int_env) c.ints[name] = {v};
  // Data the region reads as a compile-time integer, answered by the
  // same interpreter that answers a size expression. The region has
  // already resolved the indices, so what arrives is a literal read of a
  // data-only value -- nothing here depends on the region's own scope.
  c.extern_int = [&](const mir::Expr& x, long* out) {
    if (!x.data_only) return false;
    auto evaluated = try_eval_pure(x);
    if (!evaluated || !evaluated->is_int || evaluated->i.size() != 1)
      return false;
    *out = evaluated->i[0];
    return true;
  };
  c.extern_ints = [&](const mir::Expr& x, std::vector<long>* values,
                      std::vector<int64_t>* dims) {
    if (!x.data_only || x.unsized.depth == 0 ||
        x.unsized.leaf != mir::UnsizedLeaf::Int)
      return false;
    auto evaluated = try_eval_pure(x);
    if (!evaluated || !evaluated->is_int ||
        evaluated->i.size() != evaluated->r.size())
      return false;
    values->assign(evaluated->i.begin(), evaluated->i.end());
    *dims = evaluated->dims;
    return true;
  };
  c.extern_real = [&](const mir::Expr& x, double* value) {
    if (x.type_ != "UReal") return false;
    auto evaluated = try_eval_pure(x);
    if (!evaluated || evaluated->is_int || evaluated->r.size() != 1)
      return false;
    *value = evaluated->r[0];
    return true;
  };
  c.lower_higher_order = [&](const mir::Expr& x, Range* result) {
    return lower_program_higher_order(c, x, result);
  };
  if (!in_write_array) {
    c.bind_target = [&](Range* r) {
      const int slot = current_target_slot();
      r->reg = c.alloc(1);
      r->len = 1;
      prog->ins.push_back(IslandProg::LiveIn{r->reg, 1});
      reg->in_slots.push_back(slot);
      return true;
    };
  }
  std::set<std::string> outer_names;
  for (const auto& [name, value] : scope) outer_names.insert(name);
  for (const auto& [name, value] : decls) outer_names.insert(name);
  const std::set<std::string> outer_int_names = int_locals;
  c.bind_extern = [&](const std::string& name, Range* r) {
    auto sc = scope.find(name);
    int slot = sc != scope.end() ? sc->second.slot : env_slot(name);
    if (slot < 0) slot = uninitialized_decl_slot(name);
    if (slot < 0) return false;
    const int64_t len = g.slots[slot].len;
    r->reg = c.alloc((int)len);
    r->len = (int)len;
    const SlotInfo& si = scope.at(name).si;
    r->rows = si.rows;
    r->cols = si.cols;
    r->kind = si.kind;
    if (is_array(si)) {
      const ArrayShape& arr = array_shape(si);
      r->dims = arr.dims;
      r->leaf = arr.leaf;
    }
    if (len > 0) {
      prog->ins.push_back(IslandProg::LiveIn{r->reg, (int)len});
      reg->in_slots.push_back(slot);
    }
    return true;
  };
  // `target +=` inside the region accumulates into a register of its
  // own, seeded to zero, and the total leaves as one more live-out that
  // lowering registers as a target term. A `~` statement cannot go here
  // (its dropped constants depend on argument types the program binds
  // uniformly), and stanc lowers `~` to TargetPE with the propto form
  // already chosen, so the compiler refuses what it cannot reproduce.
  int target_reg = -1;
  if (s) {
    target_reg = c.alloc(1);
    const double zero = 0.0;
    c.emit_const(target_reg, &zero, 1);
    c.target_reg = target_reg;
  }
  try {
    if (s) {
      // A local declared before the region but never assigned has no
      // slot yet (lowering makes one on first assignment), so there is
      // no outside value to read: the region declares it itself. Stan
      // initializes a local to NaN, and an arm that does not assign it
      // has to leave it that way.
      std::vector<std::string> pre;
      assigned_names(*s, &pre);
      for (const std::string& name : pre) {
        if (scope.count(name) || int_locals.count(name)) continue;
        auto dl = decls.find(name);
        if (dl == decls.end()) continue;
        Range view;
        view.rows = dl->second.si.rows;
        view.cols = dl->second.si.cols;
        view.kind = dl->second.si.kind;
        if (is_array(dl->second.si)) {
          const ArrayShape& arr = array_shape(dl->second.si);
          view.dims = arr.dims;
          view.leaf = arr.leaf;
        }
        const double fill =
            dl->second.int_array
                ? static_cast<double>(std::numeric_limits<int>::min())
                : std::numeric_limits<double>::quiet_NaN();
        c.declare(name, (int)dl->second.len, view, fill);
      }
      c.stmt(*s);
      std::vector<std::string> assigned;
      assigned_names(*s, &assigned);
      for (const std::string& name : assigned) {
        const bool is_outer_int = outer_int_names.count(name) != 0;
        if (!outer_names.count(name) && !is_outer_int) continue;
        auto it = c.reals.find(name);
        if (it == c.reals.end()) continue;
        reg->out_names.push_back(name);
        reg->out_is_int.push_back(is_outer_int);
        reg->out_views.push_back(it->second);
        for (int k = 0; k < it->second.len; ++k)
          prog->out_regs.push_back(it->second.reg + k);
      }
      if (has_target_pe(*s)) {
        reg->has_target = true;
        prog->out_regs.push_back(target_reg);
      }
      // An integer the region folded is one this lowering holds a copy
      // of, and the copy is a compile-time constant every later size,
      // index and read would keep using. The region compiler folds only
      // what certainly happens, so the value it ends with is the one
      // every path through the region leaves behind. Nothing carries an
      // integer out of the program itself: a live-out is a register, and
      // registers hold doubles.
      for (const std::string& name : assigned) {
        auto folded = c.ints.find(name);
        auto held = int_env.find(name);
        if (folded != c.ints.end() && folded->second.size() == 1 &&
            held != int_env.end())
          held->second = folded->second[0];
      }
    } else {
      *expr_out = c.expr(*e);
      for (int k = 0; k < expr_out->len; ++k)
        prog->out_regs.push_back(expr_out->reg + k);
    }
    c.finish();
  } catch (Bail& b) {
    fail("runtime-control region: " + b.why, s ? s->raw : e->raw);
  }
  // No live-out register is legitimate when the region found live-outs
  // and every one of them is zero-width: the data made the values empty,
  // as `matrix[0, 0]` from a dimension table does, so there is nothing
  // for the program to carry out. Finding no live-out at all is the
  // mistake this catches -- a region that lost what it was to produce --
  // unless the region's entire purpose was a conditional effect: that has
  // no data output by design, its value being the output or exception.
  reg->has_effect = island_has_effect(*prog);
  if (prog->out_regs.empty() && !(e && expr_out->len == 0) &&
      (s == nullptr || reg->out_names.empty()) && !reg->has_effect)
    fail("runtime-control region produces nothing", s ? s->raw : e->raw);
  // A region with a runtime branch keeps the var replay -- reversing
  // control flow needs the structured form the flat program has already
  // lost -- so this usually declines. It is asked anyway because a region
  // can reach here branch-free: a `~` refusal or an unknown name is not
  // the only way to end up compiled.
  // The register compactor's liveness analysis is straight-line (with
  // forward branches as barriers).  A while adds a back edge, so retaining
  // the uncompact program is the correctness-first choice: a state register
  // written in one iteration is necessarily live at the next head.
  bool has_back_edge = false;
  bool has_unmodelled_ranges = false;
  for (size_t pc = 0; pc < prog->code.size(); ++pc) {
    const Program::Instr& instr = prog->code[pc];
    if (program_code_spec(instr.code).has(kProgramNoAdjoint))
      has_unmodelled_ranges = true;
    if ((instr.code == Program::JZ || instr.code == Program::JMP) &&
        instr.dst <= static_cast<int>(pc)) {
      has_back_edge = true;
    }
  }
  // The straight-line compactor derives every range width from Instr::len.
  // Structured matrix calls use that field for the result width while
  // their operands can have different widths, so retain the original
  // register numbering until those instructions carry explicit spans.
  if (!has_back_edge && !has_unmodelled_ranges) compact_island(*prog);
  prog->native_adj = gen_adjoint(*prog) && !std::getenv("STANLI_NO_NATIVE_ADJ");
  *prog_out = std::move(prog);
}
// The OP_ISLAND for a compiled region, plus one extraction per live-out.
void Lowering::emit_island(const std::shared_ptr<IslandProg>& prog,
                           const IslandRegion& reg,
                           const std::vector<int>& out_lens,
                           std::vector<int>* out_slots) {
  int64_t packed = 0;
  for (int len : out_lens) packed += len;
  Op is;
  is.opcode = OP_ISLAND;
  // Variant stays zero: kIslandSoftmax3Variant is a tagged-payload contract
  // and may only accompany Softmax3IslandProg (the graph carver creates it).
  std::vector<int> inputs = reg.in_slots;
  if (inputs.size() <= 6) {
    for (size_t k = 0; k < prog->ins.size(); ++k) {
      prog->ins[k].input = (int)k;
      prog->ins[k].offset = 0;
    }
  } else {
    // Op::in is deliberately compact. Pack just enough leading live-ins
    // to leave five ordinary descriptors; the program's LiveIn records
    // retain the individual register ranges and point into the packed one.
    const size_t packed_count = inputs.size() - 5;
    int packed = inputs[0];
    int64_t packed_len = g.slots[packed].len;
    for (size_t k = 1; k < packed_count; ++k) {
      packed_len += g.slots[inputs[k]].len;
      packed = emit_raw(OP_CONCAT2, {packed, inputs[k]}, packed_len, {}).slot;
    }
    int offset = 0;
    for (size_t k = 0; k < packed_count; ++k) {
      prog->ins[k].input = 0;
      prog->ins[k].offset = offset;
      offset += prog->ins[k].len;
    }
    std::vector<int> compact{packed};
    for (size_t k = packed_count; k < inputs.size(); ++k) {
      prog->ins[k].input = (int)compact.size();
      prog->ins[k].offset = 0;
      compact.push_back(inputs[k]);
    }
    inputs = std::move(compact);
  }
  is.n_in = (int)inputs.size();
  for (int k = 0; k < is.n_in; ++k) is.in[k] = inputs[k];
  is.out = add_slot(packed, false);
  is.udata = prog.get();
  g.udata_pool.push_back(prog);
  g.ops.push_back(is);
  int64_t off = 0;
  for (size_t k = 0; k < out_lens.size(); ++k) {
    const int len = out_lens[k];
    const Val v =
        emit_raw(len == 1 ? OP_INDEX : OP_SLICE, {is.out}, len, {}, {(int)off});
    out_slots->push_back(v.slot);
    off += len;
  }
}
// `if (<not known while building the graph>) ... else ...`
void Lowering::lower_runtime_ifelse(const mir::Stmt& s) {
  IslandRegion reg;
  std::shared_ptr<IslandProg> prog;
  Range ignored;
  lower_island(&s, nullptr, &reg, &ignored, &prog);
  // Widths come from the region compiler's own registers: they are what
  // out_regs packs, and they already reflect a zero-length sentinel
  // declaration the region's assignment sized.
  std::vector<int> out_lens;
  for (const Range& v : reg.out_views) out_lens.push_back(v.len);
  if (reg.has_target) out_lens.push_back(1);
  // Nothing to carry out and no target to accumulate: every live-out is
  // zero-width, so the region has no observable effect and its values
  // keep the empty shape they already have outside. A `target +=` would
  // have put its own register here, so this cannot drop one -- and a
  // print()/reject() have no live-out by design, so it cannot either.
  if (prog->out_regs.empty() && !reg.has_effect) return;
  std::vector<int> out_slots;
  emit_island(prog, reg, out_lens, &out_slots);
  // Later statements read the island's results, not the old values.
  for (size_t k = 0; k < reg.out_names.size(); ++k) {
    const std::string& name = reg.out_names[k];
    SlotInfo si;
    if (reg.out_is_int[k]) {
      // This local was an SInt before the loop.  Its loop-carried value is
      // now a register-program result; retain the UInt type but make it a
      // graph-local runtime value so later branches and scalar reads use
      // the value the loop actually produced.
      si = view_of("UInt");
      si.param_free = false;
      scope[name] = Val{out_slots[k], false, si};
      decls[name] = DeclView{1, false, si};
      int_env.erase(name);
      int_locals.erase(name);
      td.env().erase(name);
      continue;
    }
    bool shaped_outside = false;
    auto old = scope.find(name);
    if (old != scope.end()) {
      si = old->second.si;
      shaped_outside = g.slots[old->second.slot].len != 0;
    } else {
      auto dl = decls.find(name);
      if (dl != decls.end()) {
        si = dl->second.si;
        shaped_outside = dl->second.len != 0;
      }
    }
    if (!shaped_outside) {
      // The outside declaration was the inliner's zero-length sentinel;
      // the region's registers carry the real shape.
      si = SlotInfo{};
      si.rows = reg.out_views[k].rows;
      si.cols = reg.out_views[k].cols;
      si.kind = reg.out_views[k].kind;
      auto dl = decls.find(name);
      if (dl != decls.end()) {
        dl->second.len = reg.out_views[k].len;
        dl->second.si = si;
      }
    }
    // Runtime regions conservatively return parameter-dependent live-outs;
    // treating one as data without a per-output dependency proof would
    // select kernels that deliberately omit adjoints for that input.
    si.param_free = false;
    scope[name] = Val{out_slots[k], scalar_autodiff(), si};
  }
  if (reg.has_target) push_target_term(out_slots.back());
}
// `<not known while building the graph> ? a : b`
Lowering::Val Lowering::lower_runtime_ternary(const mir::Expr& e) {
  IslandRegion reg;
  std::shared_ptr<IslandProg> prog;
  Range value;
  lower_island(nullptr, &e, &reg, &value, &prog);
  std::vector<int> out_slots;
  emit_island(prog, reg, {value.len}, &out_slots);
  SlotInfo si;
  si.rows = value.rows;
  si.cols = value.cols;
  si.kind = value.kind;
  return {out_slots[0], scalar_autodiff(), si};
}
// Use the runtime-control compiler as a graph producer for higher-order
// families whose shared implementation already lives there. This keeps a
// straight-line graph call and a call under dynamic control on one callback
// binder and one kernel path instead of growing a second graph-only parser.
Lowering::Val Lowering::lower_program_expression(const mir::Expr& e) {
  IslandRegion reg;
  std::shared_ptr<IslandProg> prog;
  Range value;
  lower_island(nullptr, &e, &reg, &value, &prog);
  std::vector<int> out_slots;
  emit_island(prog, reg, {value.len}, &out_slots);
  SlotInfo si;
  if (value.kind == ViewKind::Array)
    si = array_view(value.dims, value.leaf, e.data_only);
  else {
    si = view_of(e.type_);
    si.rows = value.rows;
    si.cols = value.cols;
    si.kind = value.kind;
    si.param_free = e.data_only;
  }
  return {out_slots[0], expression_autodiff(e), si};
}
// Low-level emission for dynamic slot lists and graph scaffolding whose
// output dependency is explicit at the call site.
Lowering::Val Lowering::emit_raw(uint16_t opcode, std::vector<int> ins,
                                 int64_t out_len, SlotInfo out_si,
                                 std::vector<int> idata, int out2,
                                 bool autodiff) {
  check_fixed_input_count(ins.size(), opcode);
  Op op;
  op.opcode = opcode;
  op.out2 = out2;
  op.n_in = 0;
  for (int s : ins) op.in[op.n_in++] = s;
  return finish_emit(op, out_len, out_si, std::move(idata), autodiff);
}
// The expression seam: a pure result is parameter-free exactly when all of
// its inputs are. initializer_list avoids a temporary input-list allocation
// and makes forgetting dependency propagation impossible.
Lowering::Val Lowering::emit_value(uint16_t opcode,
                                   std::initializer_list<Val> ins,
                                   int64_t out_len, SlotInfo out_si,
                                   std::vector<int> idata, int out2) {
  check_fixed_input_count(ins.size(), opcode);
  Op op;
  op.opcode = opcode;
  op.out2 = out2;
  op.n_in = 0;
  out_si.param_free = true;
  bool autodiff = false;
  for (const Val& in : ins) {
    op.in[op.n_in++] = in.slot;
    out_si.param_free = out_si.param_free && in.si.param_free;
    autodiff = autodiff || in.autodiff;
  }
  return finish_emit(op, out_len, out_si, std::move(idata), autodiff);
}
bool Lowering::stmt_effectful(const mir::Stmt& s) {
  if (s.kind == mir::Stmt::NRFunApp && message_action(s.fn_name)) return true;
  for (const auto& e : s.fn_args)
    if (expr_effectful(e)) return true;
  if (s.has_init && expr_effectful(s.init)) return true;
  if (expr_effectful(s.rhs) || expr_effectful(s.target) ||
      expr_effectful(s.lower) || expr_effectful(s.upper) ||
      expr_effectful(s.cond))
    return true;
  for (const auto& e : s.lhs_idx)
    if (expr_effectful(e)) return true;
  for (const auto& k : s.body)
    if (stmt_effectful(k)) return true;
  return false;
}
// Repeating an expression fewer times is observable for more than RNGs:
// target() reads the accumulator, compiler-internal calls may validate or
// emit, and the callback families can hide effects in another function.
// Admit the ordinary Stan-library expression grammar and explicitly keep
// those effect-capable seams out. User functions are refused wholesale;
// proving a UDF repeatable needs its own interprocedural effect summary.
bool Lowering::repeatable_target_expr(const mir::Expr& e,
                                      const std::string& loopvar) {
  if (e.kind == mir::Expr::Unsupported || expr_references(e, loopvar))
    return false;
  if (e.kind == mir::Expr::FunApp) {
    if (e.fn_lib != mir::Expr::Lib::StanLib) return false;
    const std::string& name = e.name;
    const bool rng =
        name.size() >= 4 && name.compare(name.size() - 4, 4, "_rng") == 0;
    if (rng || mir::higher_order_call(e) || mir::stateful_intrinsic_kind(e))
      return false;
  }
  for (const auto& a : e.args)
    if (!repeatable_target_expr(a, loopvar)) return false;
  return true;
}
// Conservative statement whitelist for a loop whose only externally
// visible effect is adding iterator-independent terms to target. Locals
// declared under the loop may be initialized and updated; any assignment
// to a name from the enclosing scope refuses the rewrite.
bool Lowering::repeatable_target_stmt(const mir::Stmt& s,
                                      const std::string& loopvar,
                                      const std::set<std::string>& locals,
                                      bool* has_target) {
  const auto expression_ok = [&](const mir::Expr& e) {
    return repeatable_target_expr(e, loopvar);
  };
  switch (s.kind) {
    case mir::Stmt::Block:
    case mir::Stmt::SList:
      for (const auto& child : s.body)
        if (!repeatable_target_stmt(child, loopvar, locals, has_target))
          return false;
      return true;
    case mir::Stmt::TargetPE:
      if (!expression_ok(s.target)) return false;
      *has_target = true;
      return true;
    case mir::Stmt::Decl:
      if (s.read_transform) return false;
      for (const auto& dim : s.decl_type.dims)
        if (!expression_ok(dim)) return false;
      return !s.has_init || expression_ok(s.init);
    case mir::Stmt::Assignment:
      if (!locals.count(s.lhs) || !expression_ok(s.rhs)) return false;
      for (const auto& index : s.lhs_idx)
        if (!expression_ok(index)) return false;
      return true;
    case mir::Stmt::For:
      if (!expression_ok(s.lower) || !expression_ok(s.upper)) return false;
      for (const auto& child : s.body)
        if (!repeatable_target_stmt(child, loopvar, locals, has_target))
          return false;
      return true;
    case mir::Stmt::IfElse:
      if (!expression_ok(s.cond)) return false;
      for (const auto& arm : s.body)
        if (!repeatable_target_stmt(arm, loopvar, locals, has_target))
          return false;
      return true;
    case mir::Stmt::Skip:
      return true;
    default:
      // Checks, print/reject, while/control transfer, returns, and new
      // statement kinds all keep the ordinary per-iteration path.
      return false;
  }
}
// Ask only the MIR interpreter.  Static-shape specialization below uses
// this for selector values and for path-sensitive short-circuit decisions;
// keeping it separate from try_eval_pure prevents recursive specialization.
std::optional<DataMap::Entry> Lowering::try_eval_interpreter(
    const mir::Expr& e) {
  if (expr_effectful(e)) return std::nullopt;
  if (region_current) {
    // A pure user function can still contain a huge loop. Do not execute
    // it as a speculative control/shape probe inside a retained body.
    std::function<bool(const mir::Expr&)> calls_user = [&](const mir::Expr& x) {
      if (x.kind == mir::Expr::FunApp &&
          x.fn_lib == mir::Expr::Lib::UserDefined)
        return true;
      for (const auto& arg : x.args)
        if (calls_user(arg)) return true;
      return false;
    };
    if (calls_user(e)) return std::nullopt;
  }
  try {
    return td.eval(e);
  } catch (const CompileError&) {
    return std::nullopt;
  } catch (const std::domain_error&) {
    return std::nullopt;
  } catch (const std::invalid_argument&) {
    return std::nullopt;
  }
}
Lowering::StaticProbe<Lowering::StaticSelector> Lowering::try_static_selector(
    const mir::Expr& index, int64_t extent) {
  if (index.name == "IndexAll")
    return {StaticProbeState::Known, {extent, false}, {}};
  if (index.name == "IndexSingle" && index.args.size() == 1) {
    const auto at = try_static_int(index.args[0]);
    if (at.state != StaticProbeState::Known) return {at.state, {}, at.error};
    if (at.value < 1 || at.value > extent)
      return {
          StaticProbeState::Invalid, {}, "static matrix index out of bounds"};
    return {StaticProbeState::Known, {1, true}, {}};
  }
  if (index.name == "IndexBetween" && index.args.size() == 2) {
    const auto lo = try_static_int(index.args[0]);
    if (lo.state != StaticProbeState::Known) return {lo.state, {}, lo.error};
    const auto hi = try_static_int(index.args[1]);
    if (hi.state != StaticProbeState::Known) return {hi.state, {}, hi.error};
    // Stan's range indexing treats hi < lo as empty and performs no bounds
    // check on either endpoint (the same rule check_range implements).
    if (hi.value < lo.value) return {StaticProbeState::Known, {0, false}, {}};
    if (lo.value < 1 || hi.value > extent)
      return {
          StaticProbeState::Invalid, {}, "static matrix range out of bounds"};
    return {StaticProbeState::Known, {hi.value - lo.value + 1, false}, {}};
  }
  if (index.name == "IndexUpfrom" && index.args.size() == 1) {
    const auto lo = try_static_int(index.args[0]);
    if (lo.state != StaticProbeState::Known) return {lo.state, {}, lo.error};
    if (extent < lo.value) return {StaticProbeState::Known, {0, false}, {}};
    if (lo.value < 1)
      return {
          StaticProbeState::Invalid, {}, "static matrix range out of bounds"};
    return {StaticProbeState::Known, {extent - lo.value + 1, false}, {}};
  }
  if (index.name == "IndexMulti" && index.args.size() == 1) {
    auto evaluated = try_eval_interpreter(index.args[0]);
    if (!evaluated) return {};
    if (!evaluated->is_int || evaluated->i.size() != evaluated->r.size())
      return {StaticProbeState::Invalid,
              {},
              "static matrix gather index is not integer data"};
    for (int at : evaluated->i)
      if (at < 1 || at > extent)
        return {StaticProbeState::Invalid,
                {},
                "static matrix gather index out of bounds"};
    return {StaticProbeState::Known,
            {static_cast<int64_t>(evaluated->i.size()), false},
            {}};
  }
  return {};
}
// Logical geometry only: this probe must never materialize a data value or
// emit a graph op.  The first tranche deliberately handles the expression
// forms responsible for the ctsem false island -- named values and matrix
// subviews selected by compile-time integer data.  Everything else declines
// to the existing runtime-control path.
Lowering::StaticProbe<Lowering::StaticView> Lowering::try_static_view(
    const mir::Expr& e) {
  if (e.kind == mir::Expr::Var) {
    auto value = scope.find(e.name);
    if (value != scope.end())
      return {StaticProbeState::Known,
              {g.slots[value->second.slot].len, value->second.si},
              {}};
    auto declaration = decls.find(e.name);
    if (declaration != decls.end())
      return {StaticProbeState::Known,
              {declaration->second.len, declaration->second.si},
              {}};
    return {};
  }
  if (e.kind != mir::Expr::Indexed || e.args.size() < 2 || e.args.size() > 3)
    return {};
  const auto base = try_static_view(e.args[0]);
  if (base.state != StaticProbeState::Known)
    return {base.state, {}, base.error};
  if (!is_matrix(base.value.si)) return {};

  const auto rows = try_static_selector(e.args[1], base.value.si.rows);
  if (rows.state != StaticProbeState::Known)
    return {rows.state, {}, rows.error};
  StaticProbe<StaticSelector> cols{
      StaticProbeState::Known, {base.value.si.cols, false}, {}};
  if (e.args.size() == 3)
    cols = try_static_selector(e.args[2], base.value.si.cols);
  if (cols.state != StaticProbeState::Known)
    return {cols.state, {}, cols.error};

  const bool rd = rows.value.drops_dimension;
  const bool cd = cols.value.drops_dimension;
  StaticView out;
  out.len = checked_product({rows.value.count, cols.value.count},
                            "static matrix subview");
  out.si.param_free = base.value.si.param_free;
  if (!rd && !cd) {
    if (e.type_ != "UMatrix")
      return {StaticProbeState::Invalid,
              {},
              "static matrix subview has an inconsistent result type"};
    out.si = matrix_view(rows.value.count, cols.value.count,
                         base.value.si.param_free);
  } else if (rd && !cd) {
    if (e.type_ != "URowVector")
      return {StaticProbeState::Invalid,
              {},
              "static matrix row has an inconsistent result type"};
    out.si = view_of("URowVector");
    out.si.param_free = base.value.si.param_free;
  } else if (!rd && cd) {
    if (e.type_ != "UVector")
      return {StaticProbeState::Invalid,
              {},
              "static matrix column has an inconsistent result type"};
    out.si = view_of("UVector");
    out.si.param_free = base.value.si.param_free;
  } else {
    if (e.type_ != "UReal")
      return {StaticProbeState::Invalid,
              {},
              "static matrix element has an inconsistent result type"};
    out.si = view_of("UReal");
    out.si.param_free = base.value.si.param_free;
  }
  return {StaticProbeState::Known, out, {}};
}
Lowering::StaticProbe<int64_t> Lowering::try_static_shape_query(
    const mir::Expr& e) {
  if (!is_shape_query(e)) return {};
  const auto view = try_static_view(e.args[0]);
  if (view.state != StaticProbeState::Known) return {view.state, 0, view.error};
  const StaticView& v = view.value;
  if (is_array(v.si)) {
    const ArrayShape& shape = array_shape(v.si);
    if (e.name == "size" || e.name == "FnLength")
      return {StaticProbeState::Known, shape.dims.front(), {}};
    if (e.name == "num_elements") return {StaticProbeState::Known, v.len, {}};
    return {StaticProbeState::Invalid, 0,
            e.name + " is undefined for an array value"};
  }
  const LogicalDims dims = logical_dims(v.si, v.len, e.name);
  if (e.name == "rows") return {StaticProbeState::Known, dims.rows, {}};
  if (e.name == "cols") return {StaticProbeState::Known, dims.cols, {}};
  return {StaticProbeState::Known, v.len, {}};
}
// Replace only shape queries proven from immutable logical geometry.  The
// walk is lazy across Stan's short-circuit forms: an invalid subview in a
// dead RHS/arm must not become a bind-time error merely because this probe
// visited it.
bool Lowering::specialize_static_shapes(mir::Expr* e) {
  bool changed = false;
  if (e->kind == mir::Expr::EAnd || e->kind == mir::Expr::EOr) {
    if (e->args.size() != 2) return false;
    changed = specialize_static_shapes(&e->args[0]);
    auto lhs = try_eval_interpreter(e->args[0]);
    if (!lhs || lhs->r.size() != 1) return changed;
    const bool value = lhs->r[0] != 0.0;
    const bool reaches_rhs = e->kind == mir::Expr::EAnd ? value : !value;
    if (reaches_rhs) changed |= specialize_static_shapes(&e->args[1]);
    return changed;
  }
  if (e->kind == mir::Expr::TernaryIf) {
    if (e->args.size() != 3) return false;
    changed = specialize_static_shapes(&e->args[0]);
    auto condition = try_eval_interpreter(e->args[0]);
    if (!condition || condition->r.size() != 1) return changed;
    const size_t arm = condition->r[0] != 0.0 ? 1 : 2;
    changed |= specialize_static_shapes(&e->args[arm]);
    return changed;
  }
  if (is_shape_query(*e)) {
    const auto value = try_static_shape_query(*e);
    if (value.state == StaticProbeState::Invalid) fail(value.error, e->raw);
    if (value.state == StaticProbeState::Known) {
      if (value.value < std::numeric_limits<int>::min() ||
          value.value > std::numeric_limits<int>::max())
        fail("static shape query exceeds the Stan integer range", e->raw);
      mir::Expr literal;
      literal.kind = mir::Expr::LitInt;
      literal.lit_i = static_cast<long>(value.value);
      literal.type_ = "UInt";
      literal.unsized = {0, mir::UnsizedLeaf::Int};
      literal.data_only = true;
      literal.raw = e->raw;
      *e = std::move(literal);
      return true;
    }
  }
  for (mir::Expr& arg : e->args) changed |= specialize_static_shapes(&arg);
  return changed;
}
std::optional<Lowering::Val> Lowering::fold_const(const mir::Expr& e) {
  if (!e.data_only || e.fn_propto || expr_effectful(e)) return std::nullopt;
  auto evaluated = try_eval_pure(e);
  if (!evaluated) return std::nullopt;
  DataMap::Entry en = std::move(*evaluated);
  if (en.r.size() == 1 &&
      (e.type_ == "UReal" || e.type_ == "UInt" || e.type_ == "UComplex"))
    return constant(en.r[0]);
  SlotInfo si;
  si.param_free = true;
  if (e.unsized.depth != 0) {
    ViewKind leaf = ViewKind::Flat;
    if (e.unsized.leaf == mir::UnsizedLeaf::Vector)
      leaf = ViewKind::Vector;
    else if (e.unsized.leaf == mir::UnsizedLeaf::RowVector)
      leaf = ViewKind::RowVector;
    else if (e.unsized.leaf == mir::UnsizedLeaf::Matrix)
      leaf = ViewKind::Matrix;
    if (en.dims.empty()) en.dims = {(int64_t)en.r.size()};
    si = array_view(en.dims, leaf, true);
  } else {
    stamp_kind(&si, e.type_);
  }
  if (e.type_ == "UMatrix" && en.dims.size() == 2)
    si = matrix_view(en.dims[0], en.dims[1], true);
  const bool nested_matrix =
      e.unsized.depth != 0 && e.unsized.leaf == mir::UnsizedLeaf::Matrix;
  std::vector<double> vals =
      graph_order(en, e.type_ == "UMatrix", nested_matrix);
  const int s = add_slot((int64_t)vals.size(), false);
  out.fills.emplace_back(s, vals);
  Val v{s, false, si, owning_layout(si)};
  observe(v, std::move(en));
  return v;
}
// Matrix shape of an elementwise result: whichever operand carries one
// (both must agree when both do).
SlotInfo Lowering::shape_of(const Val& a, const Val& b) {
  const bool as = is_scalar(a), bs = is_scalar(b);
  const int64_t la = g.slots[a.slot].len, lb = g.slots[b.slot].len;
  if (!as && !bs && !same_view(a.si, la, b.si, lb))
    fail("elementwise op on different logical views");
  SlotInfo si = as && !bs ? b.si : a.si;
  // A pure op is parameter-free when both inputs are; this lets a
  // transformed data matrix still drive OP_MATVEC.
  si.param_free = a.si.param_free && b.si.param_free;
  return si;
}
// Two-argument scalar math with one int argument
// (STANLI_SCALAR_BINARY_INT_FIRST_LIST and its SECOND twin): elementwise
// with scalar broadcast like the all-real binaries, but shape_of does not
// apply. Those two sides may legitimately carry different views --
// `ldexp(matrix, array[,] int)` is a matrix, `falling_factorial(real,
// array[,] int)` is an array -- so the result takes the real side's view
// when it has one and the int side's when the real side is a scalar,
// which is what the signature list says in every case.
Lowering::Val Lowering::lower_binary_int(uint16_t opcode, bool int_first,
                                         CallArguments& actuals) {
  actuals.require_arity(2);
  const mir::Expr& e = actuals.call_expr();
  Val a = actuals.at(0).value();
  Val b = actuals.at(1).value();
  const Val& re = int_first ? b : a;
  const Val& iv = int_first ? a : b;
  const int64_t lr = g.slots[re.slot].len, li = g.slots[iv.slot].len;
  // Only a language scalar broadcasts. A one-element container against a
  // wider one is the size error stan-math throws, not a broadcast.
  if (!is_scalar(re) && !is_scalar(iv) && lr != li)
    fail(e.name + ": arguments must match in size", e.raw);
  const SlotInfo si = is_scalar(re) ? iv.si : re.si;
  // The one place the two flat orders disagree: a matrix leaf is stored
  // column-major and an int array's trailing two extents are row-major.
  // Handing the kernel that leaf's rows and cols is what tells it to undo
  // the difference; see IntLane in kernels/scalar_binary.cpp.
  std::vector<int> idata;
  if (!is_scalar(iv)) {
    if (is_matrix(re.si)) {
      idata = {(int)re.si.rows, (int)re.si.cols};
    } else if (is_array(re.si)) {
      const ArrayShape& s = array_shape(re.si);
      if (s.leaf == ViewKind::Matrix)
        idata = {(int)s.dims[s.dims.size() - 2], (int)s.dims.back()};
    }
  }
  return with_layout(
      emit_value(opcode, {a, b}, std::max(lr, li), si, std::move(idata)),
      elementwise_layout({a, b}));
}
// Value of a data-only expression at compile time. The interpreter
// handles most cases; a UDF-local constant lives only as a slot, so fall
// back to that slot's recorded fill.
std::vector<double> Lowering::const_values(const mir::Expr& e) {
  if (expr_effectful(e))
    fail("effectful expression cannot be demanded at compile time", e.raw);
  if (auto evaluated = try_eval_pure(e)) {
    DataMap::Entry en = std::move(*evaluated);
    return en.r;
  }
  Val v = lower_expr(e);
  if (const DataMap::Entry* en = observation(v)) return en->r;
  // A zero-length slot carries no values by construction (`array[0] real`
  // is how ODE models spell "no data for the system").
  if (g.slots[v.slot].len == 0) return {};
  fail("value must be known at compile time: " +
           (e.kind == mir::Expr::Var ? e.name : ("<" + e.name + ">")),
       e.raw);
}
std::vector<int> Lowering::const_ints(const mir::Expr& e) {
  if (expr_effectful(e))
    fail("effectful expression cannot be demanded as compile-time integers",
         e.raw);
  if (auto evaluated = try_eval_pure(e)) {
    DataMap::Entry en = std::move(*evaluated);
    if (en.is_int) return en.i;
    std::vector<int> out;
    for (double d : en.r) out.push_back((int)d);
    return out;
  }
  std::vector<int> out;
  for (double d : const_values(e)) out.push_back((int)d);
  return out;
}
// Availability is independent of both MIR's AD type and param_free. A
// data-only loop result lives in a slot without a compile-time observation.
// This probe does not lower or execute anything (in particular, no UDF loop).
bool Lowering::needs_runtime_value(const mir::Expr& e) {
  if (is_shape_query(e) &&
      try_static_shape_query(e).state == StaticProbeState::Known)
    return false;
  if (e.kind == mir::Expr::Var) {
    const auto v = scope.find(e.name);
    return v != scope.end() && !observation(v->second) &&
           (!v->second.si.param_free ||
            (!int_env.count(e.name) && !td.find(e.name)));
  }
  if (expr_effectful(e)) return true;
  for (const auto& arg : e.args)
    if (needs_runtime_value(arg)) return true;
  return false;
}
bool Lowering::runtime_int_value(const mir::Expr& e) const {
  if (e.type_ != "UInt" || e.unsized.leaf != mir::UnsizedLeaf::Int ||
      e.unsized.depth != 0)
    return false;
  if (e.kind == mir::Expr::Var) {
    auto it = scope.find(e.name);
    return it != scope.end() && !it->second.si.param_free;
  }
  if (e.kind == mir::Expr::Indexed && !e.args.empty() &&
      e.args[0].kind == mir::Expr::Var) {
    auto it = scope.find(e.args[0].name);
    return it != scope.end() && !it->second.si.param_free;
  }
  return false;
}
Lowering::Val Lowering::lower_runtime_int_sum(const mir::Expr& e,
                                              CallArguments& actuals) {
  if (!in_write_array)
    fail("runtime integer sum is supported only in generated quantities",
         e.raw);
  if (!is_int_sum_surface(e))
    fail(
        "runtime integer sum needs one one-dimensional int-array argument "
        "and a scalar int result",
        e.raw);

  actuals.require_arity(1);
  Val a = actuals.at(0).value();
  if (!is_array(a.si))
    fail("runtime integer sum argument is not an array", e.raw);
  const ArrayShape& shape = array_shape(a.si);
  const int64_t len = g.slots[a.slot].len;
  if (shape.leaf != ViewKind::Flat || shape.dims.size() != 1)
    fail("runtime integer sum needs a one-dimensional int array", e.raw);
  if (len <= 0) fail("runtime integer sum needs a nonempty int array", e.raw);
  if (a.si.param_free)
    fail("runtime integer sum needs a runtime-produced int array", e.raw);

  const auto initialized = int_initialized_prefix.find(a.slot);
  if (initialized == int_initialized_prefix.end() || initialized->second != len)
    fail("runtime integer sum array is not definitely initialized", e.raw);
  const auto known = int_ranges.find(a.slot);
  if (known == int_ranges.end())
    fail("runtime integer sum has unproved integral slot values", e.raw);
  const IntRange range = known->second;
  const uint64_t n = static_cast<uint64_t>(len);
  if (range.lo < 0) {
    const uint64_t magnitude =
        static_cast<uint64_t>(-static_cast<int64_t>(range.lo));
    const uint64_t capacity = static_cast<uint64_t>(
        -static_cast<int64_t>(std::numeric_limits<int32_t>::min()));
    if (n > capacity / magnitude)
      fail("runtime integer sum may overflow int32 in a partial sum", e.raw);
  }
  if (range.hi > 0 &&
      n > static_cast<uint64_t>(std::numeric_limits<int32_t>::max()) /
              static_cast<uint64_t>(range.hi))
    fail("runtime integer sum may overflow int32 in a partial sum", e.raw);

  Val result = with_layout(emit_value(OP_SUM_VEC, {a}, 1, view_of("UInt")),
                           ExpressionLayout::scalar());
  result.autodiff = false;
  // A range is only a static proof; the source itself was required to be
  // runtime-produced.  Keeping this result non-constant prevents later
  // compile-time geometry/control from consuming it through Val metadata.
  result.si.param_free = false;
  set_int_range(result, static_cast<int64_t>(range.lo) * len,
                static_cast<int64_t>(range.hi) * len);
  return result;
}
// ---- statements -----------------------------------------------------------
CompiledModel::ParamView Lowering::parameter_view(const mir::Stmt& s, int slot,
                                                  int64_t len) {
  using Naming = CompiledModel::ParamView::Naming;
  CompiledModel::ParamView view{s.decl_id, slot, len};
  if (s.decl_type.base == "SReal" || s.decl_type.base == "SInt" ||
      s.decl_type.base == "SComplex") {
    view.naming = Naming::Scalar;
    return view;
  }
  view.naming = Naming::Container;
  std::vector<int64_t> dims;
  for (const auto& dim : s.decl_type.dims) dims.push_back(eval_int(dim));
  int64_t declared_len = 1;
  for (int64_t dim : dims) declared_len *= dim;
  if (declared_len != len)
    fail("constrained shape does not match its flattened length", s.raw);
  const bool matrix_storage =
      s.decl_type.base == "SMatrix" ||
      (s.decl_type.base == "SArray" && s.decl_type.elem_base == "SMatrix");
  view.set_serial_layout(std::move(dims), matrix_storage);
  if (matrix_storage) {
    view.naming = Naming::Matrix;
    view.rows = view.dims.at(view.dims.size() - 2);
  }
  return view;
}
void Lowering::lower_read_param(const mir::Stmt& s) {
  const mir::Transform& tr = *s.read_transform;
  const std::vector<int64_t> declared_dims = sized_dims(s.decl_type);
  const ViewKind leaf = s.decl_type.base == "SArray"
                            ? leaf_kind(s.decl_type.elem_base)
                            : leaf_kind(s.decl_type.base);
  const size_t leaf_dims = static_cast<size_t>(leaf_rank(leaf));
  if (declared_dims.size() < leaf_dims)
    fail("parameter declaration has incomplete leaf dimensions", s.raw);
  const std::vector<int64_t> outer_dims(declared_dims.begin(),
                                        declared_dims.end() - leaf_dims);
  const int64_t n_batch = checked_product(outer_dims, "parameter batch");

  // Unstructured transforms use FnReadParam's declared raw shape. A
  // structured leaf replaces only its innermost dimensions below; the
  // outer array geometry remains declaration-owned and orthogonal.
  std::vector<int64_t> raw_dims;
  for (const auto& d : s.read_dims) raw_dims.push_back(eval_int(d));
  std::vector<int64_t> expected_read_dims = declared_dims;
  if (tr.kind == mir::Transform::CholeskyCorr ||
      tr.kind == mir::Transform::Correlation ||
      tr.kind == mir::Transform::Covariance) {
    if (leaf != ViewKind::Matrix || expected_read_dims.size() < 2)
      fail("matrix parameter transform has a non-matrix declaration", s.raw);
    expected_read_dims.pop_back();
  }
  if (raw_dims != expected_read_dims)
    fail("parameter read dimensions do not match its declaration", s.raw);
  int64_t con_len = checked_product(raw_dims, "parameter read shape");
  int64_t raw_len = con_len;
  int64_t inner_raw = 0, inner_con = 0;
  int64_t matrix_rows = 0, matrix_cols = 0;

  auto vector_leaf = [&](int64_t free_size) {
    if (leaf != ViewKind::Vector || declared_dims.empty())
      fail("vector parameter transform has a non-vector declaration", s.raw);
    inner_raw = free_size;
    inner_con = declared_dims.back();
    raw_dims = outer_dims;
    raw_dims.push_back(inner_raw);
    raw_len = checked_product({n_batch, inner_raw}, "parameter raw shape");
    con_len =
        checked_product({n_batch, inner_con}, "parameter constrained shape");
  };
  auto flat_matrix_leaf = [&](int64_t free_size) {
    if (leaf != ViewKind::Matrix || declared_dims.size() < 2)
      fail("matrix parameter transform has a non-matrix declaration", s.raw);
    matrix_rows = declared_dims[declared_dims.size() - 2];
    matrix_cols = declared_dims.back();
    inner_raw = free_size;
    inner_con =
        checked_product({matrix_rows, matrix_cols}, "parameter matrix leaf");
    raw_dims = outer_dims;
    raw_dims.push_back(inner_raw);
    raw_len = checked_product({n_batch, inner_raw}, "parameter raw shape");
    con_len =
        checked_product({n_batch, inner_con}, "parameter constrained shape");
  };

  if (tr.kind == mir::Transform::Simplex ||
      tr.kind == mir::Transform::SumToZero) {
    if (leaf == ViewKind::Vector) {
      vector_leaf(declared_dims.back() - 1);
    } else if (tr.kind == mir::Transform::SumToZero &&
               leaf == ViewKind::Matrix) {
      matrix_rows = declared_dims[declared_dims.size() - 2];
      matrix_cols = declared_dims.back();
      inner_raw = checked_product({matrix_rows - 1, matrix_cols - 1},
                                  "sum-to-zero matrix raw shape");
      inner_con = checked_product({matrix_rows, matrix_cols},
                                  "sum-to-zero matrix leaf");
      raw_dims = outer_dims;
      raw_dims.push_back(matrix_rows - 1);
      raw_dims.push_back(matrix_cols - 1);
      raw_len = checked_product({n_batch, inner_raw}, "parameter raw shape");
      con_len =
          checked_product({n_batch, inner_con}, "parameter constrained shape");
    } else {
      fail("sum-to-zero or simplex transform has an invalid declaration",
           s.raw);
    }
  } else if (tr.kind == mir::Transform::Ordered ||
             tr.kind == mir::Transform::PositiveOrdered ||
             tr.kind == mir::Transform::UnitVector) {
    if (leaf != ViewKind::Vector || declared_dims.empty())
      fail("vector parameter transform has a non-vector declaration", s.raw);
    vector_leaf(declared_dims.back());
  } else if (tr.kind == mir::Transform::CholeskyCorr ||
             tr.kind == mir::Transform::Correlation ||
             tr.kind == mir::Transform::Covariance) {
    if (leaf != ViewKind::Matrix || declared_dims.size() < 2)
      fail("matrix parameter transform has a non-matrix declaration", s.raw);
    const int64_t K = declared_dims.back();
    if (declared_dims[declared_dims.size() - 2] != K)
      fail("square matrix transform has a rectangular declaration", s.raw);
    const int64_t free_size = tr.kind == mir::Transform::Covariance
                                  ? K * (K + 1) / 2
                                  : K * (K - 1) / 2;
    flat_matrix_leaf(free_size);
  } else if (tr.kind == mir::Transform::CholeskyCov) {
    if (leaf != ViewKind::Matrix || declared_dims.size() < 2)
      fail("matrix parameter transform has a non-matrix declaration", s.raw);
    const int64_t M = declared_dims[declared_dims.size() - 2];
    const int64_t N = declared_dims.back();
    if (M < N) fail("cholesky-factor-cov rows are smaller than columns", s.raw);
    flat_matrix_leaf(N * (N + 1) / 2 + (M - N) * N);
  }

  SlotInfo psi = view_of(s.decl_type);
  const int64_t declared_len = sized_len(s.decl_type);
  if (declared_len != con_len)
    fail("parameter view length does not match constrained storage", s.raw);
  validate_view(psi, con_len, "parameter " + s.decl_id);
  decls[s.decl_id] = DeclView{con_len, scalar_autodiff(), psi};
  const int raw = add_slot(raw_len, /*is_param=*/true);
  out.param_names.push_back(s.decl_id);
  {
    // The unconstrained layout the caller needs to slice a draw: how
    // long this parameter's piece is, and what it means. raw_len and
    // con_len part company for every structured transform.
    CompiledModel::UncParam u;
    u.name = s.decl_id;
    u.len = raw_len;
    u.dims = raw_dims;
    u.transform = tr.kind;
    out.unc_params.push_back(std::move(u));
  }
  out.n_unconstrained += raw_len;

  if (tr.kind == mir::Transform::Identity) {
    Val value{raw, scalar_autodiff(), psi, owning_layout(psi)};
    CompiledModel::ParamView serial_view = parameter_view(s, raw, raw_len);
    scope[s.decl_id] = value;
    // In write_array mode the column order is dictated by the FnWriteParam
    // statements, which come later and cover transformed parameters and
    // generated quantities too; declaration order would be wrong.
    if (!in_write_array) {
      serial_view.slot = value.slot;
      out.views.push_back(std::move(serial_view));
    }
    return;
  }
  uint16_t opcode = 0;
  std::vector<int> ins{raw};
  switch (tr.kind) {
    case mir::Transform::Lower:
      opcode = OP_CONSTRAIN_LOWER;
      ins.push_back(lower_expr(tr.args[0]).slot);
      break;
    case mir::Transform::Upper:
      opcode = OP_CONSTRAIN_UPPER;
      ins.push_back(lower_expr(tr.args[0]).slot);
      break;
    case mir::Transform::LowerUpper:
      opcode = OP_CONSTRAIN_LU;
      ins.push_back(lower_expr(tr.args[0]).slot);
      ins.push_back(lower_expr(tr.args[1]).slot);
      break;
    case mir::Transform::CholeskyCorr:
      opcode = OP_CONSTRAIN_CHOL_CORR;
      break;
    case mir::Transform::Simplex:
      opcode = OP_CONSTRAIN_SIMPLEX;
      break;
    case mir::Transform::Ordered:
      opcode = OP_CONSTRAIN_ORDERED;
      break;
    case mir::Transform::PositiveOrdered:
      opcode = OP_CONSTRAIN_POS_ORDERED;
      break;
    // offset / multiplier: the affine transform, and the modern
    // non-centering idiom. stanc3 emits three tags depending on which
    // halves were written, so the missing half becomes its identity
    // (offset 0, multiplier 1) and one kernel serves all three.
    case mir::Transform::Offset:
    case mir::Transform::Multiplier:
    case mir::Transform::OffsetMultiplier: {
      opcode = OP_CONSTRAIN_OFFSET_MULT;
      const int zero = const_slot(0.0);
      const int one = const_slot(1.0);
      if (tr.kind == mir::Transform::Offset) {
        ins.push_back(lower_expr(tr.args[0]).slot);
        ins.push_back(one);
      } else if (tr.kind == mir::Transform::Multiplier) {
        ins.push_back(zero);
        ins.push_back(lower_expr(tr.args[0]).slot);
      } else {
        ins.push_back(lower_expr(tr.args[0]).slot);
        ins.push_back(lower_expr(tr.args[1]).slot);
      }
      break;
    }
    case mir::Transform::UnitVector:
      opcode = OP_CONSTRAIN_UNIT_VECTOR;
      break;
    case mir::Transform::SumToZero:
      opcode = leaf == ViewKind::Matrix ? OP_CONSTRAIN_SUM_TO_ZERO_MAT
                                        : OP_CONSTRAIN_SUM_TO_ZERO;
      break;
    case mir::Transform::Correlation:
      opcode = OP_CONSTRAIN_CORR_MATRIX;
      break;
    case mir::Transform::Covariance:
      opcode = OP_CONSTRAIN_COV_MATRIX;
      break;
    case mir::Transform::CholeskyCov:
      opcode = OP_CONSTRAIN_CHOL_COV;
      break;
    default:
      fail("unsupported parameter transform", tr.raw);
  }
  const int jac = add_slot(1, false);
  std::vector<int> tr_idata;
  if (opcode == OP_CONSTRAIN_SIMPLEX || opcode == OP_CONSTRAIN_ORDERED ||
      opcode == OP_CONSTRAIN_POS_ORDERED ||
      opcode == OP_CONSTRAIN_UNIT_VECTOR || opcode == OP_CONSTRAIN_SUM_TO_ZERO)
    tr_idata = {checked_immediate(n_batch, "structured parameter batch"),
                checked_immediate(inner_raw, "structured raw leaf"),
                checked_immediate(inner_con, "structured constrained leaf")};
  if (opcode == OP_CONSTRAIN_CHOL_CORR || opcode == OP_CONSTRAIN_CORR_MATRIX ||
      opcode == OP_CONSTRAIN_COV_MATRIX || opcode == OP_CONSTRAIN_CHOL_COV)
    tr_idata = {checked_immediate(n_batch, "structured parameter batch"),
                checked_immediate(inner_raw, "structured raw leaf"),
                checked_immediate(matrix_rows, "structured matrix rows"),
                checked_immediate(matrix_cols, "structured matrix columns")};
  if (opcode == OP_CONSTRAIN_SUM_TO_ZERO_MAT) {
    tr_idata = {checked_immediate(n_batch, "structured parameter batch"),
                checked_immediate(inner_raw, "structured raw leaf"),
                checked_immediate(matrix_rows, "structured matrix rows"),
                checked_immediate(matrix_cols, "structured matrix columns")};
  }
  Val con =
      emit_raw(opcode, ins, con_len, psi, tr_idata, jac, scalar_autodiff());
  con.layout = owning_layout(psi);
  jac_slots.push_back(jac);
  scope[s.decl_id] = con;
  if (!in_write_array)
    out.views.push_back(parameter_view(s, con.slot, con_len));
}
void Lowering::lower_stmt_impl(const mir::Stmt& s) {
  switch (s.kind) {
    case mir::Stmt::Decl:
      if (s.read_transform) {
        lower_read_param(s);
      } else if (s.decl_type.base == "SInt") {
        if (s.has_init && in_write_array && runtime_int_binding(s.init)) {
          bind_runtime_int(s.decl_id, s.init, s.raw);
          return;
        }
        // A fresh scalar-int declaration shadows every representation of
        // an earlier declaration with the same optimized MIR id.  In
        // particular, a preceding runtime sum may have installed a graph
        // value in scope/decls; leaving it there would make a later Var
        // read win over the compile-time literal installed below.
        scope.erase(s.decl_id);
        decls.erase(s.decl_id);
        td.env().erase(s.decl_id);
        int_env.erase(s.decl_id);
        int_locals.erase(s.decl_id);
        // Only compile-time integers belong in int_env. MIR's DataOnly
        // AD level alone does not make a parameter-selected integer known.
        int_locals.insert(s.decl_id);
        // eval_int, not the interpreter directly: the initializer may be
        // a shape query on a slot-bound value (rows(lscale) inside an
        // inlined function), which only eval_int can answer.
        if (s.has_init) {
          if (auto value = static_int(s.init))
            int_env[s.decl_id] = *value;
          else
            bind_runtime_int(s.decl_id, s.init, s.raw);
        }
      } else if (s.decl_type.base.empty() &&
                 s.decl_type.unsized.leaf != mir::UnsizedLeaf::Unknown) {
        // O1 introduces unsized container temporaries for expressions such
        // as a for-loop sequence built with append_array. C++ assignment
        // gives these locals the RHS shape, so delay allocating or checking
        // their view until the first whole-variable assignment does likewise.
        scope.erase(s.decl_id);
        DeclView sh;
        sh.autodiff = s.decl_type.unsized.leaf != mir::UnsizedLeaf::Int &&
                      !s.decl_data_only && scalar_autodiff();
        sh.int_array = s.decl_type.unsized.leaf == mir::UnsizedLeaf::Int;
        sh.deferred_shape = true;
        if (s.has_init) {
          Val v = lower_expr(s.init);
          sh.len = g.slots[v.slot].len;
          sh.si = v.si;
          sh.deferred_shape = false;
          v.autodiff = sh.autodiff;
          v.layout = owning_layout(v.si);
          scope[s.decl_id] = v;
          sync_data_local(s.decl_id, s.init, v);
        } else {
          td.env().erase(s.decl_id);
        }
        decls[s.decl_id] = sh;
      } else {
        // A redeclaration shadows whatever the name held: --O1 inlining
        // reuses one symbol for a callee's local across loop iterations,
        // and its size can differ per iteration. The stale binding must
        // not constrain the fresh variable's width.
        scope.erase(s.decl_id);
        DeclView sh;
        sh.len = sized_len(s.decl_type);
        sh.autodiff = !s.decl_data_only && scalar_autodiff();
        sh.si = view_of(s.decl_type);
        // CmdStan fills every uninitialized integer container with the
        // INT_MIN sentinel.  Runtime-sum provenance remains deliberately
        // one-dimensional, but the value-level initialization contract is
        // independent of rank.
        sh.int_array =
            s.decl_type.base == "SArray" && s.decl_type.elem_base == "SInt";
        if (s.has_init) {
          Val v = lower_expr(s.init);
          SlotInfo expected = view_of(s.decl_type, v.si.param_free);
          require_binding(v, sh.len, expected, s.decl_id, s.raw);
          v.autodiff = sh.autodiff;
          v.si = expected;
          v.layout = owning_layout(v.si);
          scope[s.decl_id] = v;
        }
        decls[s.decl_id] = sh;
        if (s.has_init)
          sync_data_local(s.decl_id, s.init, scope.at(s.decl_id));
        else
          td.env().erase(s.decl_id);
      }
      return;
    case mir::Stmt::Assignment: {
      if (s.lhs_idx.empty() && int_locals.count(s.lhs)) {
        if (in_write_array && runtime_int_binding(s.rhs)) {
          bind_runtime_int(s.lhs, s.rhs, s.raw);
          return;
        }
        if (auto value = static_int(s.rhs))
          int_env[s.lhs] = *value;
        else
          bind_runtime_int(s.lhs, s.rhs, s.raw);
        return;
      }
      if (!s.lhs_idx.empty()) {
        // Element write under unrolled control flow: functional update.
        Val prev_v{-1, false, {}};
        auto it = scope.find(s.lhs);
        if (it != scope.end()) {
          prev_v = it->second;
        } else {
          auto dl = decls.find(s.lhs);
          if (dl == decls.end())
            fail("indexed assignment to undeclared " + s.lhs);
          SlotInfo si = dl->second.si;
          si.param_free = true;
          prev_v = Val{add_slot(dl->second.len, false), dl->second.autodiff, si,
                       owning_layout(si)};
          const double initial =
              dl->second.int_array
                  ? static_cast<double>(std::numeric_limits<int>::min())
                  : std::numeric_limits<double>::quiet_NaN();
          out.fills.emplace_back(prev_v.slot,
                                 std::vector<double>(dl->second.len, initial));
          if (dl->second.int_array) set_uninitialized_int_array(prev_v);
          observe_fill(prev_v, dl->second.int_array, initial, dl->second.len);
        }
        const int prev = prev_v.slot;
        bool all_single = true;
        for (const auto& ix : s.lhs_idx)
          if (ix.name != "IndexSingle") all_single = false;
        const std::vector<int64_t>* dd =
            is_array(prev_v.si) ? &array_shape(prev_v.si).dims : nullptr;
        const Val rhs_v = lower_expr(s.rhs);
        if (std::any_of(
                s.lhs_idx.begin(), s.lhs_idx.end(),
                [&](const mir::Expr& ix) { return runtime_selector(ix); })) {
          Val nv = region_index(prev_v, s.lhs_idx, s.rhs.type_, s.rhs.unsized,
                                &rhs_v);
          nv.autodiff = prev_v.autodiff;
          scope[s.lhs] = nv;
          sync_indexed_data_local(s.lhs, nv);
          return;
        }
        observe_indexed_rhs(s.rhs, rhs_v);
        const int rhs = rhs_v.slot;
        SlotInfo out_si = prev_v.si;
        // A one-index All spans the complete logical value. Keep this as
        // an indexed functional update rather than silently rewriting the
        // MIR statement: the ordinary binding checks still enforce width
        // and logical view, while the store path preserves integer-array
        // initialization and observation metadata. Matrix `[:, j]` is a
        // separate two-index form below and never enters this branch.
        if (s.lhs_idx.size() == 1 && s.lhs_idx[0].name == "IndexAll") {
          if (is_scalar(prev_v))
            fail("full-span assignment needs a container for " + s.lhs, s.raw);
          require_binding(rhs_v, g.slots[prev].len, prev_v.si, s.lhs, s.raw);
          Val nv = with_layout(emit_value(OP_SET_SLICE, {prev_v, rhs_v},
                                          g.slots[prev].len, out_si, {0}),
                               owning_layout(out_si));
          propagate_int_update(nv, prev_v, rhs_v, 0, 1);
          scope[s.lhs] = nv;
          sync_indexed_data_local(s.lhs, nv);
          return;
        }
        // Whole matrix row write M[i] = row_vector: one value per column,
        // strided by the physical row count.
        if (s.lhs_idx.size() == 1 && s.lhs_idx[0].name == "IndexSingle" &&
            is_matrix(prev_v.si) && is_row_vector(rhs_v.si)) {
          const int64_t i = eval_int(s.lhs_idx[0].args[0]) - 1;
          if (i < 0 || i >= prev_v.si.rows)
            fail("row assignment index out of bounds for " + s.lhs);
          if (g.slots[rhs].len != prev_v.si.cols)
            fail("row assignment size mismatch for " + s.lhs);
          Val nv = with_layout(emit_value(OP_SET_SLICE_STRIDED, {prev_v, rhs_v},
                                          g.slots[prev].len, out_si,
                                          {(int)i, (int)prev_v.si.rows}),
                               owning_layout(out_si));
          propagate_int_update(nv, prev_v, rhs_v, i, prev_v.si.rows);
          scope[s.lhs] = nv;
          sync_indexed_data_local(s.lhs, nv);
          return;
        }
        // Whole vector leaf write A[i, :] = rhs for array[N] vector[S].
        // Graph array storage keeps each outer element contiguous, so this
        // is the assignment mirror of the read path above.
        if (s.lhs_idx.size() == 2 && s.lhs_idx[0].name == "IndexSingle" &&
            s.lhs_idx[1].name == "IndexAll" && dd && dd->size() == 2 &&
            (array_shape(prev_v.si).leaf == ViewKind::Vector ||
             array_shape(prev_v.si).leaf == ViewKind::RowVector)) {
          const int64_t i = eval_int(s.lhs_idx[0].args[0]);
          const int64_t width = (*dd)[1];
          check_index(i, (*dd)[0], "array assignment index", s.raw);
          SlotInfo expected = indexed_view(prev_v.si, 1, width, s.rhs.type_);
          require_binding(rhs_v, width, expected, s.lhs, s.raw);
          const int64_t start = (i - 1) * width;
          Val nv =
              with_layout(emit_value(OP_SET_SLICE, {prev_v, rhs_v},
                                     g.slots[prev].len, out_si, {(int)start}),
                          owning_layout(out_si));
          propagate_int_update(nv, prev_v, rhs_v, start, 1);
          scope[s.lhs] = nv;
          sync_indexed_data_local(s.lhs, nv);
          return;
        }
        // Between write w[a:b] = rhs (contiguous on 1-D values).
        if (s.lhs_idx.size() == 1 && is_range(s.lhs_idx[0])) {
          const bool flat_1d_array =
              is_array(prev_v.si) && array_shape(prev_v.si).dims.size() == 1 &&
              array_shape(prev_v.si).leaf == ViewKind::Flat;
          if (!is_vector(prev_v.si) && !is_row_vector(prev_v.si) &&
              !flat_1d_array)
            fail("range assignment needs a one-dimensional flat value for " +
                     s.lhs,
                 s.raw);
          const StaticRange range =
              *static_range(s.lhs_idx[0], g.slots[prev].len);
          const int64_t lo = range.lo;
          const int64_t hi = range.hi;
          const int64_t len = hi >= lo ? hi - lo + 1 : 0;
          check_range(lo, hi, g.slots[prev].len, "range assignment", s.raw);
          if (g.slots[rhs].len != len)
            fail("range assignment size mismatch for " + s.lhs);
          const int64_t start = len == 0 ? 0 : lo - 1;
          Val nv =
              with_layout(emit_value(OP_SET_SLICE, {prev_v, rhs_v},
                                     g.slots[prev].len, out_si, {(int)start}),
                          owning_layout(out_si));
          propagate_int_update(nv, prev_v, rhs_v, start, 1);
          scope[s.lhs] = nv;
          sync_indexed_data_local(s.lhs, nv);
          return;
        }
        // Scatter write x[idx] = rhs. The indices are data, so spell it as
        // one element write each; repeats then resolve last-wins as CmdStan.
        if (s.lhs_idx.size() == 1 && s.lhs_idx[0].name == "IndexMulti" &&
            !is_matrix(prev_v.si)) {
          DataMap::Entry iv =
              eval_pure(s.lhs_idx[0].args[0], "a scatter index");
          if (!iv.is_int) fail("scatter index must be int data", s.raw);
          if ((int64_t)iv.i.size() != g.slots[rhs].len)
            fail("scatter assignment size mismatch for " + s.lhs);
          Val nv = prev_v;
          for (size_t k = 0; k < iv.i.size(); ++k) {
            check_index(iv.i[k], g.slots[prev].len, "scatter index", s.raw);
            const Val el =
                emit_value(OP_INDEX, {rhs_v}, 1, view_of("UReal"), {(int)k});
            const Val next =
                emit_value(OP_SET_INDEX, {nv, el}, g.slots[prev].len, out_si,
                           {(int)(iv.i[k] - 1)});
            propagate_int_update(next, nv, el, iv.i[k] - 1, 1);
            nv = next;
          }
          scope[s.lhs] = nv;
          sync_indexed_data_local(s.lhs, nv);
          return;
        }
        // Column write M[:, j] = rhs (contiguous in col-major storage).
        if (s.lhs_idx.size() == 2 && s.lhs_idx[0].name == "IndexAll" &&
            s.lhs_idx[1].name == "IndexSingle" && is_matrix(prev_v.si)) {
          const int64_t j = eval_int(s.lhs_idx[1].args[0]) - 1;
          if (j < 0 || j >= prev_v.si.cols)
            fail("column assignment index out of bounds for " + s.lhs);
          if (g.slots[rhs].len != prev_v.si.rows)
            fail("column assignment size mismatch for " + s.lhs);
          Val nv = emit_value(OP_SET_SLICE, {prev_v, rhs_v}, g.slots[prev].len,
                              out_si, {(int)(j * prev_v.si.rows)});
          propagate_int_update(nv, prev_v, rhs_v, j * prev_v.si.rows, 1);
          scope[s.lhs] = nv;
          sync_indexed_data_local(s.lhs, nv);
          return;
        }
        // Row-range column write M[a:b, j] = rhs (contiguous within the
        // column).
        if (s.lhs_idx.size() == 2 && is_range(s.lhs_idx[0]) &&
            s.lhs_idx[1].name == "IndexSingle" && is_matrix(prev_v.si)) {
          const StaticRange range = *static_range(s.lhs_idx[0], prev_v.si.rows);
          const int64_t lo = range.lo;
          const int64_t hi = range.hi;
          const int64_t j = eval_int(s.lhs_idx[1].args[0]) - 1;
          if (j < 0 || j >= prev_v.si.cols)
            fail("column assignment index out of bounds for " + s.lhs);
          const int64_t len = hi >= lo ? hi - lo + 1 : 0;
          check_range(lo, hi, prev_v.si.rows, "row-range assignment", s.raw);
          if (g.slots[rhs].len != len)
            fail("range assignment size mismatch for " + s.lhs);
          const int64_t start = len == 0 ? 0 : j * prev_v.si.rows + lo - 1;
          Val nv =
              with_layout(emit_value(OP_SET_SLICE, {prev_v, rhs_v},
                                     g.slots[prev].len, out_si, {(int)start}),
                          owning_layout(out_si));
          propagate_int_update(nv, prev_v, rhs_v, start, 1);
          scope[s.lhs] = nv;
          sync_indexed_data_local(s.lhs, nv);
          return;
        }
        // Columns outermost, as CmdStan's assign walks them: a repeated
        // index has to resolve last-wins in the same order.
        if (!all_single && s.lhs_idx.size() == 2 && is_matrix(prev_v.si)) {
          const std::vector<int64_t> ri = index_positions(
              s.lhs_idx[0], prev_v.si.rows, "block assignment row", s.raw);
          const std::vector<int64_t> ci = index_positions(
              s.lhs_idx[1], prev_v.si.cols, "block assignment column", s.raw);
          if ((int64_t)(ri.size() * ci.size()) != g.slots[rhs].len)
            fail("block assignment size mismatch for " + s.lhs, s.raw);
          Val nv = prev_v;
          for (size_t j = 0; j < ci.size(); ++j)
            for (size_t i = 0; i < ri.size(); ++i) {
              const Val el = emit_value(OP_INDEX, {rhs_v}, 1, view_of("UReal"),
                                        {(int)(j * ri.size() + i)});
              const Val next =
                  emit_value(OP_SET_INDEX, {nv, el}, g.slots[prev].len, out_si,
                             {(int)(ci[j] * prev_v.si.rows + ri[i])});
              propagate_int_update(next, nv, el, ci[j] * prev_v.si.rows + ri[i],
                                   1);
              nv = next;
            }
          scope[s.lhs] = nv;
          sync_indexed_data_local(s.lhs, nv);
          return;
        }
        if (all_single && dd && s.lhs_idx.size() <= dd->size() &&
            !is_matrix(prev_v.si)) {
          // The mirror of the read path, through the same flat_addr.
          const auto& D = *dd;
          const bool mat = array_shape(prev_v.si).leaf == ViewKind::Matrix;
          std::vector<int64_t> ix;
          for (const auto& k : s.lhs_idx) ix.push_back(eval_int(k.args[0]) - 1);
          const Addr a = flat_addr(D, mat, ix);
          if (a.len != g.slots[rhs].len && a.len != 1)
            fail("indexed assignment size mismatch for " + s.lhs);
          Val nv =
              a.stride != 1
                  ? emit_value(OP_SET_SLICE_STRIDED, {prev_v, rhs_v},
                               g.slots[prev].len, out_si,
                               {(int)a.off, (int)a.stride})
                  : (a.len == 1
                         ? emit_value(OP_SET_INDEX, {prev_v, rhs_v},
                                      g.slots[prev].len, out_si, {(int)a.off})
                         : emit_value(OP_SET_SLICE, {prev_v, rhs_v},
                                      g.slots[prev].len, out_si, {(int)a.off}));
          propagate_int_update(nv, prev_v, rhs_v, a.off, a.stride);
          scope[s.lhs] = nv;
          sync_indexed_data_local(s.lhs, nv);
          return;
        }
        // A full array-index prefix followed by an explicit `:` for every
        // remaining dimension: H[i, :, :] on array[N] matrix[R, C] (a
        // container leaf), or y_approx[i, :] on a plain array[N, S] real
        // (the remaining dimension is just another array axis, no
        // container leaf at all) -- either way this spells the same
        // whole-remainder replacement flat_addr's "whole elements" case
        // already gives an implicit-rest prefix. Not `all_single` (the
        // trailing indices are All, not omitted or Single), so it falls
        // outside the block above.
        if (dd) {
          size_t prefix_len = 0;
          while (prefix_len < s.lhs_idx.size() &&
                 s.lhs_idx[prefix_len].name == "IndexSingle")
            ++prefix_len;
          bool trailing_all = true;
          for (size_t d = prefix_len; d < s.lhs_idx.size(); ++d)
            if (s.lhs_idx[d].name != "IndexAll") trailing_all = false;
          if (prefix_len > 0 && trailing_all && prefix_len < dd->size() &&
              s.lhs_idx.size() == dd->size()) {
            std::vector<int64_t> ix;
            ix.reserve(prefix_len);
            for (size_t d = 0; d < prefix_len; ++d) {
              const int64_t one = eval_int(s.lhs_idx[d].args[0]);
              check_index(one, (*dd)[d], "array assignment index", s.raw);
              ix.push_back(one - 1);
            }
            const bool mat = array_shape(prev_v.si).leaf == ViewKind::Matrix;
            const Addr a = flat_addr(*dd, mat, ix);
            require_binding(
                rhs_v, a.len,
                indexed_view(prev_v.si, prefix_len, a.len, s.rhs.type_), s.lhs,
                s.raw);
            Val nv = emit_value(OP_SET_SLICE, {prev_v, rhs_v},
                                g.slots[prev].len, out_si, {(int)a.off});
            propagate_int_update(nv, prev_v, rhs_v, a.off, 1);
            scope[s.lhs] = nv;
            sync_indexed_data_local(s.lhs, nv);
            return;
          }
        }
        int64_t flat = 0;
        if (all_single && s.lhs_idx.size() == 1) {
          flat = eval_int(s.lhs_idx[0].args[0]) - 1;
        } else if (all_single && s.lhs_idx.size() == 2 &&
                   is_matrix(prev_v.si)) {
          flat = (eval_int(s.lhs_idx[1].args[0]) - 1) * prev_v.si.rows +
                 (eval_int(s.lhs_idx[0].args[0]) - 1);
        } else {
          std::string desc = "unsupported indexed assignment: lhs=" + s.lhs;
          for (const auto& ix : s.lhs_idx)
            desc += " [" + (ix.name.empty() ? "?" : ix.name) + "]";
          fail(desc, s.raw);
        }
        Val nv = with_layout(emit_value(OP_SET_INDEX, {prev_v, rhs_v},
                                        g.slots[prev].len, out_si, {(int)flat}),
                             owning_layout(out_si));
        propagate_int_update(nv, prev_v, rhs_v, flat, 1);
        scope[s.lhs] = nv;
        sync_indexed_data_local(s.lhs, nv);
        return;
      }
      {
        Val rhs = lower_expr(s.rhs);
        auto old = scope.find(s.lhs);
        if (old != scope.end()) {
          require_binding(rhs, g.slots[old->second.slot].len, old->second.si,
                          s.lhs, s.raw);
          const bool param_free = rhs.si.param_free;
          rhs.autodiff = old->second.autodiff;
          rhs.si = old->second.si;
          rhs.si.param_free = param_free;
        } else {
          auto dl = decls.find(s.lhs);
          if (dl != decls.end()) {
            if (dl->second.deferred_shape) {
              dl->second.len = g.slots[rhs.slot].len;
              dl->second.si = rhs.si;
              dl->second.deferred_shape = false;
            } else if (dl->second.len == 0 &&
                       (g.slots[rhs.slot].len != 0 ||
                        (is_matrix(dl->second.si) && is_matrix(rhs.si) &&
                         (dl->second.si.rows != rhs.si.rows ||
                          dl->second.si.cols != rhs.si.cols)))) {
              // stanc3's --O1 inliner declares a function's return
              // variable zero-length (`array[real, 0]`, `vector[0]`)
              // because the returned size is the callee's business, and
              // C++ assignment resizes. Slots do not, so the first
              // whole-variable assignment defines the shape instead.
              dl->second.len = g.slots[rhs.slot].len;
              dl->second.si = rhs.si;
            } else {
              SlotInfo expected = dl->second.si;
              require_binding(rhs, dl->second.len, expected, s.lhs, s.raw);
              const bool pf = rhs.si.param_free;
              rhs.si = expected;
              rhs.si.param_free = pf;
            }
            rhs.autodiff = dl->second.autodiff;
          }
        }
        rhs.layout = owning_layout(rhs.si);
        scope[s.lhs] = rhs;
        sync_data_local(s.lhs, s.rhs, rhs);
      }
      return;
    }
    case mir::Stmt::TargetPE: {
      // Stan defines `target += e` for a container `e` as adding `sum(e)`
      // -- CmdStan's `lp_accum__.add(e)` reduces the whole container. A
      // target term is consumed as a scalar, so the reduction has to
      // happen here; pushing the container's slot would silently
      // contribute element zero alone.
      Val t = lower_expr(s.target);
      if (g.slots[t.slot].len != 1) t = emit_value(OP_SUM_VEC, {t}, 1);
      push_target_term(t.slot);
      return;
    }
    case mir::Stmt::Block:
    case mir::Stmt::SList: {
      if (!write_array_known_static && in_write_array &&
          needs_runtime_control(s)) {
        lower_runtime_ifelse(s);
        return;
      }
      const bool outer_known_static = write_array_known_static;
      write_array_known_static = true;
      try {
        for (const auto& k : s.body) lower_stmt(k);
      } catch (...) {
        write_array_known_static = outer_known_static;
        throw;
      }
      write_array_known_static = outer_known_static;
      return;
    }
    case mir::Stmt::Skip:
      return;
    case mir::Stmt::NRFunApp:
      if (s.fn_name == "FnCheck") {
        // prepare_data checks already ran in bind_data. Any check reaching
        // this lowering belongs to log_prob/write_array and must retain its
        // per-evaluation position, even when its value is parameter-free.
        if (!s.check_transform) fail("malformed FnCheck", s.raw);
        if (mir::is_structured_check(s.check_transform->kind)) {
          if (!s.check_transform->args.empty() || s.fn_args.size() != 1)
            fail("malformed structured FnCheck", s.raw);
          const Val value = lower_expr(s.fn_args[0]);
          const int64_t value_len = g.slots[value.slot].len;
          validate_view(value.si, value_len, "structured FnCheck value");

          auto spec = std::make_shared<StructuredCheckSpec>();
          spec->kind = s.check_transform->kind;
          spec->name =
              s.check_var_name.empty() ? s.fn_args[0].name : s.check_var_name;
          if (is_array(value.si)) {
            const ArrayShape& shape = array_shape(value.si);
            spec->dims = shape.dims;
            if (shape.leaf == ViewKind::Vector)
              spec->leaf = StructuredLeaf::Vector;
            else if (shape.leaf == ViewKind::Matrix)
              spec->leaf = StructuredLeaf::Matrix;
            else
              fail("structured FnCheck requires vector or matrix leaves",
                   s.raw);
          } else if (is_vector(value.si)) {
            spec->dims = {value_len};
            spec->leaf = StructuredLeaf::Vector;
          } else if (is_matrix(value.si)) {
            spec->dims = {value.si.rows, value.si.cols};
            spec->leaf = StructuredLeaf::Matrix;
          } else {
            fail("structured FnCheck requires a vector or matrix", s.raw);
          }

          const size_t leaf_rank = spec->leaf == StructuredLeaf::Matrix ? 2 : 1;
          const mir::UnsizedLeaf expr_leaf = s.fn_args[0].unsized.leaf;
          if (s.fn_args[0].unsized.depth != spec->dims.size() - leaf_rank ||
              (spec->leaf == StructuredLeaf::Vector &&
               expr_leaf != mir::UnsizedLeaf::Vector) ||
              (spec->leaf == StructuredLeaf::Matrix &&
               expr_leaf != mir::UnsizedLeaf::Matrix))
            fail("structured FnCheck type does not match its value", s.raw);
          const bool matrix_only = spec->kind == mir::Transform::CholeskyCorr ||
                                   spec->kind == mir::Transform::Correlation ||
                                   spec->kind == mir::Transform::Covariance ||
                                   spec->kind == mir::Transform::CholeskyCov;
          const bool vector_only =
              spec->kind != mir::Transform::SumToZero && !matrix_only;
          if ((matrix_only && spec->leaf != StructuredLeaf::Matrix) ||
              (vector_only && spec->leaf != StructuredLeaf::Vector))
            fail("structured FnCheck transform and leaf disagree", s.raw);

          (void)emit_value(OP_CHECK_STRUCTURED, {value}, 1);
          g.ops.back().udata = spec.get();
          g.udata_pool.push_back(std::move(spec));
          return;
        }
        if (s.check_transform->args.size() != 1 || s.fn_args.size() != 2)
          fail("malformed FnCheck", s.raw);
        const uint16_t opcode =
            s.check_transform->kind == mir::Transform::Lower   ? OP_CHECK_LOWER
            : s.check_transform->kind == mir::Transform::Upper ? OP_CHECK_UPPER
                                                               : 0;
        if (opcode == 0) fail("unsupported FnCheck transform", s.raw);

        const Val value = lower_expr(s.fn_args[0]);
        const Val bound = lower_expr(s.fn_args[1]);
        const int64_t value_len = g.slots[value.slot].len;
        const int64_t bound_len = g.slots[bound.slot].len;
        validate_view(value.si, value_len, "FnCheck value");
        validate_view(bound.si, bound_len, "FnCheck bound");
        const bool bound_is_scalar = is_scalar(bound);
        const bool shapes_match =
            is_scalar(value)
                ? bound_is_scalar
                : (bound_is_scalar ||
                   same_view(value.si, value_len, bound.si, bound_len));

        auto spec = std::make_shared<BoundCheckSpec>();
        spec->name =
            s.check_var_name.empty() ? s.fn_args[0].name : s.check_var_name;
        spec->bound_is_scalar = bound_is_scalar;
        spec->shapes_match = shapes_match;
        (void)emit_value(opcode, {value, bound}, 1);
        g.ops.back().udata = spec.get();
        g.udata_pool.push_back(std::move(spec));
        return;
      }
      // Size validation remains a separate compatibility seam.
      if (s.fn_name == "FnValidateSize") return;
      if (s.fn_name == "check_matching_dims") {
        if (s.fn_args.size() != 5 || s.fn_args[0].kind != mir::Expr::LitStr ||
            s.fn_args[1].kind != mir::Expr::LitStr ||
            s.fn_args[3].kind != mir::Expr::LitStr)
          fail("malformed check_matching_dims", s.raw);
        const Val value = lower_expr(s.fn_args[2]);
        const Val bound = lower_expr(s.fn_args[4]);
        const int64_t value_len = g.slots[value.slot].len;
        const int64_t bound_len = g.slots[bound.slot].len;
        validate_view(value.si, value_len, "check_matching_dims value");
        validate_view(bound.si, bound_len, "check_matching_dims bound");
        auto spec = std::make_shared<BoundCheckSpec>();
        spec->name = s.fn_args[1].lit_s;
        spec->shapes_match =
            same_view(value.si, value_len, bound.si, bound_len);
        (void)emit_value(OP_CHECK_MATCHING_DIMS, {value, bound}, 1);
        g.ops.back().udata = spec.get();
        g.udata_pool.push_back(std::move(spec));
        return;
      }
      // Deliberately not a `check_*` prefix match: a value check like
      // check_positive_finite rejects a draw at runtime, and skipping one
      // would silently accept points CmdStan refuses.
      // reject() and print(): the message is a mix of string literals
      // and expressions, so the literals become the op's chunk list and
      // the expressions become its inputs. reject throws
      // std::domain_error at forward time, which is the same exception
      // from the same place CmdStan's generated code throws it, so the
      // sampler counts it as a rejected proposal rather than a failure.
      if (const auto action = message_action(s.fn_name)) {
        auto spec = std::make_shared<MessageSpec>();
        std::vector<int> ins;
        *spec =
            lower_message_arguments(s.fn_args, [&](const mir::Expr& argument) {
              // Op::in holds six. Keep that backend capacity check here;
              // parsing and semantic dispatch remain shared.
              if (ins.size() >= 6)
                fail(std::string(*action == MessageAction::Reject ? "reject"
                                                                  : "print") +
                         " with more than 6 printed values",
                     s.raw);
              ins.push_back(lower_expr(argument).slot);
            });
        Op op;
        op.opcode = *action == MessageAction::Reject ? OP_REJECT : OP_PRINT;
        op.n_in = (int)ins.size();
        for (size_t k = 0; k < ins.size(); ++k) op.in[k] = ins[k];
        // The output is a dead scalar: every op writes somewhere, and
        // nothing reads this one.
        op.out = add_slot(1, false);
        op.udata = spec.get();
        g.udata_pool.push_back(spec);
        g.ops.push_back(op);
        return;
      }
      if (s.fn_name == "FnWriteParam") {
        // One CSV column, at the point the emission happens: this is what
        // fixes the column order to CmdStan's. Arrays of containers are
        // emitted one element at a time -- `array[K] simplex[K] theta`
        // arrives as K writes of `theta[k]` -- and CmdStan names those
        // columns outer-index-first, theta.1.1 .. theta.1.K, theta.2.1 ...
        // so the index path becomes part of the column name.
        if (s.fn_args.size() != 1) fail("FnWriteParam arity", s.raw);
        std::vector<long> ixs;
        const mir::Expr* base = &s.fn_args[0];
        while (base->kind == mir::Expr::Indexed) {
          for (size_t k = base->args.size(); k-- > 1;) {
            if (base->args[k].name != "IndexSingle")
              fail("FnWriteParam under a non-scalar index", s.raw);
            ixs.push_back(eval_int(base->args[k].args[0]));
          }
          base = &base->args[0];
        }
        if (base->kind != mir::Expr::Var)
          fail("FnWriteParam of a non-variable", s.raw);
        std::string name = base->name;
        for (auto it = ixs.rbegin(); it != ixs.rend(); ++it)
          name += "." + std::to_string(*it);
        const Val v = lower_expr(s.fn_args[0]);
        // stanc peels the array dimensions, so what is left here is a
        // scalar, a vector/row_vector, or a matrix -- and its type decides
        // how CmdStan indexes the columns.
        using Naming = CompiledModel::ParamView::Naming;
        const std::string& t = s.fn_args[0].type_;
        CompiledModel::ParamView pv{name, v.slot, g.slots[v.slot].len};
        if (t == "UReal" || t == "UInt" || t == "UComplex") {
          pv.naming = Naming::Scalar;
        } else if (t == "UMatrix") {
          if (!is_matrix(v.si))
            fail("FnWriteParam of a matrix with unknown shape: " + name, s.raw);
          pv.rows = v.si.rows;
          pv.naming = Naming::Matrix;
        } else {
          pv.naming = Naming::Container;
        }
        out.views.push_back(pv);
        return;
      }
      fail("unsupported statement function " + s.fn_name);
    case mir::Stmt::For: {
      long lo = 0, hi = 0;
      try {
        lo = eval_int(s.lower);
        hi = eval_int(s.upper);
      } catch (const CompileError&) {
        if (in_write_array ||
            !(needs_runtime_value(s.lower) || needs_runtime_value(s.upper)) ||
            !try_lower_region(s))
          throw;
        return;
      }
      if (lo > hi) {
        int_env.erase(s.loopvar);
        return;
      }
      // Both the pre-control target fold and the ordinary path ask the same
      // structural question.  A nonselected automatic candidate reaches
      // both sites, so retain the answer for this lowering encounter rather
      // than walking a potentially large body twice.
      std::optional<bool> repeatable_target;
      const auto has_repeatable_target = [&]() {
        if (!repeatable_target) repeatable_target = repeatable_target_body(s);
        return *repeatable_target;
      };
      // Both cheap invariant folding and retained selection precede the
      // per-iteration control scan. Neither needs an expanded graph.
      if ((structured_policy == StructuredMode::Prefer ||
           structured_policy == StructuredMode::Force) &&
          lo != hi && has_repeatable_target()) {
        const double old_scale = target_scale;
        target_scale *= static_cast<double>(hi) - static_cast<double>(lo) + 1;
        int_env[s.loopvar] = lo;
        try {
          for (const auto& child : s.body) lower_stmt(child);
        } catch (...) {
          target_scale = old_scale;
          int_env.erase(s.loopvar);
          throw;
        }
        target_scale = old_scale;
        int_env.erase(s.loopvar);
        return;
      }
      if (try_lower_region(s, std::pair<int64_t, int64_t>{lo, hi})) return;
      // runtime_loop_control evaluates data-only conditions while looking
      // for a parameter-selected break/continue. Scan under the same loop
      // binding that ordinary unrolling will use: without it, an indexed
      // condition such as idx[ri] is either treated as spuriously dynamic
      // or can escape static-shape specialization as an unknown variable.
      // The bounds come first so a zero-trip loop never evaluates its body.
      const auto old = int_env.find(s.loopvar);
      const bool had_old = old != int_env.end();
      const long old_value = had_old ? old->second : 0;
      bool has_runtime_loop_control = false;
      try {
        for (long v = lo; v <= hi && !has_runtime_loop_control; ++v) {
          int_env[s.loopvar] = v;
          for (const auto& child : s.body)
            if (runtime_loop_control(child)) {
              has_runtime_loop_control = true;
              break;
            }
        }
      } catch (...) {
        if (had_old)
          int_env[s.loopvar] = old_value;
        else
          int_env.erase(s.loopvar);
        throw;
      }
      if (had_old)
        int_env[s.loopvar] = old_value;
      else
        int_env.erase(s.loopvar);
      if (has_runtime_loop_control) {
        lower_runtime_ifelse(s);
        return;
      }
      if (lo != hi && has_repeatable_target()) {
        const double old_scale = target_scale;
        target_scale *= static_cast<double>(hi) - static_cast<double>(lo) + 1.0;
        int_env[s.loopvar] = lo;
        try {
          for (const auto& child : s.body) lower_stmt(child);
        } catch (...) {
          target_scale = old_scale;
          int_env.erase(s.loopvar);
          throw;
        }
        target_scale = old_scale;
        int_env.erase(s.loopvar);
        return;
      }
      for (long v = lo; v <= hi; ++v) {
        int_env[s.loopvar] = v;
        try {
          for (const auto& k : s.body) lower_stmt(k);
        } catch (LoopContinue&) {
          continue;
        } catch (LoopBreak&) {
          break;
        }
      }
      int_env.erase(s.loopvar);
      return;
    }
    case mir::Stmt::While: {
      if (try_lower_region(s)) return;
      // Unlike `for`, a `while` has no statically supplied trip count.
      // Compile it as one structured register-program island, which
      // rechecks its guard at execution time and replays the executed
      // iterations under autodiff.  This deliberately has no lowering-time
      // iteration cap: nontermination is the model's runtime behaviour,
      // not a reason to silently truncate or reject a finite long loop.
      lower_runtime_ifelse(s);
      return;
    }
    case mir::Stmt::IfElse: {
      // The guards are data-only and fold away below (both flags are
      // pinned on), so this is the only chance to note that a CSV
      // section ended here.
      if (in_write_array) {
        switch (mir::emit_guard(s)) {
          case mir::EmitGuard::TransformedParams:
            if (!n_tp_start) n_tp_start = out.views.size();
            break;
          case mir::EmitGuard::GeneratedQuantities:
            if (!n_gq_start) n_gq_start = out.views.size();
            break;
          case mir::EmitGuard::None:
            break;
        }
      }
      bool known = false, c = false;
      if (auto evaluated = try_eval_pure(s.cond)) {
        c = evaluated->r.at(0) != 0.0;
        known = true;
      }
      if (known) {
        if (c && !s.body.empty()) lower_stmt(s.body[0]);
        if (!c && s.body.size() > 1) lower_stmt(s.body[1]);
        return;
      }
      if (udf_depth > 0 && s.body.size() == 2) {
        mir::Stmt effects = s;
        mir::Expr then_value, else_value;
        if (peel_terminal_return(&effects.body[0], &then_value) &&
            peel_terminal_return(&effects.body[1], &else_value)) {
          std::vector<std::string> assigned;
          assigned_names(effects, &assigned);
          if (!assigned.empty() || has_target_pe(effects) ||
              stmt_effectful(effects))
            lower_runtime_ifelse(effects);

          mir::Expr choice;
          choice.kind = mir::Expr::TernaryIf;
          choice.args = {s.cond, then_value, else_value};
          choice.type_ = then_value.type_;
          choice.unsized = then_value.unsized;
          choice.data_only =
              s.cond.data_only && then_value.data_only && else_value.data_only;
          choice.raw = s.raw;
          throw LpReturn{lower_expr(choice)};
        }
      }
      // Data-only or not, an unfoldable condition compiles to an island.
      // Data-only says the MIR adlevel is DataOnly, not that the values are
      // in the interpreter's frame: a UDF local built by indexed assignment
      // lives in the graph, and only the region compiler can read it there.
      // The island's live-outs come back parameter-dependent, which costs
      // adjoints such a branch does not need but is never wrong.
      lower_runtime_ifelse(s);
      return;
    }
    case mir::Stmt::Return:
      // Only reachable inside an inlined UDF body (log_prob itself has no
      // value returns); unwinds to lower_call_udf.
      if (!s.has_init) fail("void return unsupported in UDF inlining");
      throw LpReturn{lower_expr(s.rhs)};
    case mir::Stmt::Break:
      throw LoopBreak{};
    case mir::Stmt::Continue:
      throw LoopContinue{};
    default:
      fail("unsupported statement", s.raw);
  }
}
// Scalar terms reduce through chained ADD_N ops (6-input limit per op).
int Lowering::reduce_terms(std::vector<int> terms) {
  // The target is a scalar, and every consumer of a term reads one value
  // from it. A container term is therefore not a shape to accommodate but
  // a lowering bug -- one whose symptom, before this check, was a model
  // that sampled a wrong posterior without saying anything.
  for (int t : terms)
    if (g.slots[t].len != 1) fail("target term is not a scalar");
  if (terms.empty()) return const_slot(0.0);
  while (terms.size() > 1) {
    std::vector<int> next;
    for (size_t i = 0; i < terms.size(); i += 6) {
      const size_t n = std::min<size_t>(6, terms.size() - i);
      if (n == 1) {
        next.push_back(terms[i]);
        continue;
      }
      std::vector<int> chunk(terms.begin() + i, terms.begin() + i + n);
      next.push_back(emit_raw(OP_ADD_N, chunk, 1, {}).slot);
    }
    terms = std::move(next);
  }
  return terms[0];
}
// Shared tail of both lowerings: inplace/store-forward/reroll always run;
// the rest is gated by plan so write_array can skip the passes that assume
// a scalar log-density result. Ordering constraints between the stages
// that do run are noted where each stage starts.
void Lowering::run_passes(const std::vector<int>& roots, const PassPlan& plan) {
  const auto trace = [&](const char* stage, PrepTrace::Time from,
                         PrepTrace::Extra extra = PrepTrace::Extra::None,
                         int64_t a = 0, int64_t b = 0, bool deep = false,
                         int64_t params = 0, int64_t c = 0, int64_t d = 0,
                         const detail::RerollDispositionStats* disp = nullptr) {
    prep.graph(prep_graph, stage, from, g, out.fills, target_terms.size(),
               out.views.size(), extra, a, b, deep, params, c, d, disp);
  };

  // Target terms have no consuming op yet either: reduce_terms (log_prob)
  // or the arena reads (write_array) see them only after the passes run.
  std::vector<int> update_roots = roots;
  update_roots.insert(update_roots.end(), target_terms.begin(),
                      target_terms.end());
  const auto inplace_time = prep.start();
  const int inplace =
      make_inplace_updates(g, update_roots);  // off under STANLI_NO_INPLACE
  trace("inplace", inplace_time, PrepTrace::Extra::Rewrites, inplace);
  // Deleting the write/read-back pairs first is what leaves a plain
  // arithmetic lane for reroll to vectorize.
  const auto forward_time = prep.start();
  const int forwarded = forward_stores_to_loads(g, update_roots);
  trace("store_forward", forward_time, PrepTrace::Extra::Removed, forwarded);
  if (plan.constfold) {
    // After the update chains collapse, so a data-only chain is one slot
    // rather than N; before reroll, so the lanes it sees have data
    // operands.
    const auto constfold_time = prep.start();
    const ConstFoldStats constfolded = const_fold(g, out.fills, update_roots);
    trace("constfold", constfold_time, PrepTrace::Extra::ConstFold,
          constfolded.ops_removed, constfolded.slots_folded);
  }
  const auto reroll_time = prep.start();
  RerollStats rerolled;
  detail::RerollDispositionStats reroll_dispositions;
  if (prep.enabled()) {
    detail::ProfiledRerollStats profiled =
        detail::reroll_profiled(g, out.fills, target_terms, roots);
    rerolled = profiled.work;
    reroll_dispositions = profiled.dispositions;
  } else {
    rerolled = reroll(g, out.fills, target_terms, roots);  // STANLI_NO_REROLL
  }
  trace("reroll", reroll_time, PrepTrace::Extra::Reroll, rerolled.regions,
        rerolled.list_steps, false, 0, rerolled.candidate_steps,
        rerolled.row_steps, &reroll_dispositions);
  // Re-roll can replace many element writes with copying slice stores, and
  // may have replaced target terms with vector reductions: rebuild the
  // implicit-root set before giving those new ops the same last-use proof
  // as the scalar stores.
  std::vector<int> post_reroll_roots = roots;
  post_reroll_roots.insert(post_reroll_roots.end(), target_terms.begin(),
                           target_terms.end());
  const auto post_reroll_inplace_time = prep.start();
  const int post_reroll_inplace =
      rerolled.regions ? make_inplace_updates(g, post_reroll_roots) : 0;
  trace("post_reroll_inplace", post_reroll_inplace_time,
        PrepTrace::Extra::Rewrites, post_reroll_inplace);
  std::vector<int> current_roots = post_reroll_roots;
  if (plan.partition) {
    // After re-roll, which keeps first crack at the contiguous shapes it
    // already handles, and before CSE, which would merge ops shared
    // between lanes and leave the lanes no longer whole.
    const auto partition_time = prep.start();
    const PartitionStats parted =
        partition_lanes(g, out.fills, target_terms, roots);
    trace("partition", partition_time, PrepTrace::Extra::Partition,
          parted.groups, parted.lanes, false, 0, parted.declined,
          parted.list_steps);
    // Same proof the slice stores re-roll makes get: rebuilt from the
    // terms partition just replaced.
    std::vector<int> post_partition_roots = roots;
    post_partition_roots.insert(post_partition_roots.end(),
                                target_terms.begin(), target_terms.end());
    const auto post_partition_inplace_time = prep.start();
    const int post_partition_inplace =
        parted.groups ? make_inplace_updates(g, post_partition_roots) : 0;
    trace("post_partition_inplace", post_partition_inplace_time,
          PrepTrace::Extra::Rewrites, post_partition_inplace);
    current_roots = post_partition_roots;
  }
  if (plan.elide_stores) {
    // After every pass that emits a slice store, and before islands,
    // whose bodies name outer slots in a payload this rename cannot
    // reach.
    const auto elide_time = prep.start();
    const int elided = elide_full_extent_stores(g, current_roots);
    trace("elide_stores", elide_time, PrepTrace::Extra::Removed, elided);
  }
  if (plan.cse) {
    // After reroll, whose lane matching needs the repeated ops it hoists
    // to still be there, and before islands, so they compile the smaller
    // residue.
    const auto cse_time = prep.start();
    const CseStats cse_st = cse(g, out.fills, target_terms, roots);
    trace("cse", cse_time, PrepTrace::Extra::Removed, cse_st.ops_removed);
  }
  if (plan.island) {
    // LAST, after every other pass has had first crack: compile whatever
    // scalar residue survives (recurrences the re-roll can never widen)
    // into island ops. Off under STANLI_NO_ISLAND.
    const auto island_time = prep.start();
    const int islands = carve_islands(g, out.fills, target_terms, roots);
    trace("island", island_time, PrepTrace::Extra::Regions, islands);
  }
}
CompiledModel::WriteArray Lowering::run_write_array(const mir::Program& p) {
  const auto total_time = prep.start();
  for (const auto& f : p.fun_defs) fun_defs[f.name] = &f;
  in_write_array = true;
  // stanc3 guards the two emission groups on these flags; the sampler wants
  // both, so pin them and let the data-only IfElse fold them away.
  int_env["emit_transformed_parameters__"] = 1;
  int_env["emit_generated_quantities__"] = 1;
  CompiledModel::WriteArray wa;
  const auto lower_time = prep.start();
  try {
    for (const auto& s : p.generate_quantities) lower_stmt(s);
  } catch (const CompileError& e) {
    // Keep the valid prefix for diagnostics, but drivers select WaInterp
    // whenever this marker is set and it evaluates the whole section from
    // statement zero. There is no continuation frame for an arbitrary
    // nested failure or its lexical live-outs.
    wa.truncated = e.what();
  }
  std::vector<int> roots = jac_slots;
  for (const auto& v : out.views) roots.push_back(v.slot);
  prep.graph(prep_graph, "lower", lower_time, g, out.fills, target_terms.size(),
             out.views.size(), PrepTrace::Extra::Truncated,
             !wa.truncated.empty());

  run_passes(roots, PassPlan{true, false, false, true, false});

  const auto finalize_time = prep.start();
  // Nothing reads a result here, but forward() asserts a scalar result
  // slot, so point it at one.
  g.result_slot = const_slot(0.0);
  wa.n_unconstrained = out.n_unconstrained;
  prep.graph(prep_graph, "finalize", finalize_time, g, out.fills,
             target_terms.size(), out.views.size());
  prep.graph(prep_graph, "total", total_time, g, out.fills, target_terms.size(),
             out.views.size(), PrepTrace::Extra::None, 0, 0, true,
             out.n_unconstrained);
  wa.graph = std::move(g);
  // A section stanc did not emit a guard for (or one lowering stopped
  // short of) has no columns of its own: it starts where the CSV ends.
  // The transformed-parameter boundary falls back to the generated
  // quantities one rather than to the end, so a missing first guard
  // cannot order the two backwards and hand a reader a negative count.
  wa.n_gq_start = n_gq_start.value_or(out.views.size());
  wa.n_tp_start = n_tp_start.value_or(wa.n_gq_start);
  wa.columns = std::move(out.views);
  wa.fills = std::move(out.fills);
  return wa;
}
CompiledModel Lowering::run(const mir::Program& p) {
  const auto total_time = prep.start();
  for (const auto& f : p.fun_defs) fun_defs[f.name] = &f;
  const auto bind_time = prep.start();
  bind_data(p);
  prep.graph(prep_graph, "bind_data", bind_time, g, out.fills,
             target_terms.size(), out.views.size());
  const auto lower_time = prep.start();
  for (const auto& s : p.log_prob) lower_stmt(s);
  prep.graph(prep_graph, "lower", lower_time, g, out.fills, target_terms.size(),
             out.views.size());
  // Jacobian terms and constrained-parameter views are read straight out
  // of the arena, so no op consumes them and the pass cannot infer them.
  std::vector<int> roots = jac_slots;
  for (const auto& v : out.views) roots.push_back(v.slot);

  run_passes(roots, PassPlan{true, true, true, true, true});

  const auto reduce_time = prep.start();
  std::vector<int> all = target_terms;
  all.insert(all.end(), jac_slots.begin(), jac_slots.end());
  g.result_slot = reduce_terms(all);
  prep.graph(prep_graph, "reduce", reduce_time, g, out.fills,
             target_terms.size(), out.views.size());
  prep.graph(prep_graph, "total", total_time, g, out.fills, target_terms.size(),
             out.views.size(), PrepTrace::Extra::None, 0, 0, true,
             out.n_unconstrained);
  out.graph = std::move(g);
  return std::move(out);
}
}  // namespace lower_detail

using namespace lower_detail;

CompiledModel compile_model(const std::string& mir_text, const DataMap& data) {
  const char* prep_env = std::getenv("STANLI_PROFILE_PREP");
  PrepTrace prep(prep_env && prep_env[0] != '0');
  const auto compile_time = prep.start();
  // Shared because the interpreted write_array fallback, when needed,
  // keeps the generate_quantities statements and UDF bodies alive for the
  // model's whole life.
  const auto parse_time = prep.start();
  auto prog = std::make_shared<mir::Program>(decode_program(mir_text));
  prep.plain("compile", "parse_mir", parse_time, PrepTrace::Extra::MirBytes,
             static_cast<int64_t>(mir_text.size()));
  Lowering lo(data, prep, "log_prob");
  CompiledModel cm = lo.run(*prog);
  if (!prog->generate_quantities.empty()) {
    // A second lowering, over the transformed data the first one already
    // interpreted: re-running prepare_data would double preparation time on
    // the models where preparation is the cost (nn_rbm1bJ100, 20.7 s).
    Lowering wa(data, prep, "write_array", lo.shape_pool);
    const auto env_copy_time = prep.start();
    wa.td.env() = lo.td.env();
    wa.int_env = lo.int_env_data;
    // bind_data owns immutable declaration shape and physical-layout facts;
    // write_array skips that expensive pass, so its fresh lexical lowering
    // receives the facts together with the already-prepared environment.
    wa.decls = lo.decls;
    prep.plain("write_array", "env_copy", env_copy_time);
    CompiledModel::WriteArray w = wa.run_write_array(*prog);
    if (w.n_unconstrained != cm.n_unconstrained) {
      // The two graphs read the same draw; if they disagree on its length the
      // write_array cannot be driven at all. Keep the model, drop the columns,
      // and say so rather than emitting a silently misaligned CSV.
      w.truncated = "write_array reads " + std::to_string(w.n_unconstrained) +
                    " unconstrained values, log_prob reads " +
                    std::to_string(cm.n_unconstrained) +
                    (w.truncated.empty() ? "" : "; " + w.truncated);
      w.columns.clear();
      w.n_tp_start = w.n_gq_start = 0;
    }
    // STANLI_WA_FORCE_INTERP is a TEST-ONLY hook: it attaches the
    // interpreter beside a graph that lowered the whole section, so the
    // cross-path harness (tests/cross_path.hpp) can read both engines off
    // one model and hold them against each other on the same draw. It
    // changes which objects are retained, never what either engine
    // computes -- the graph above is built identically either way. Never
    // set it in a shipped environment: drivers PREFER an attached
    // interpreter (capi.cpp, bridgestan_abi.cpp), so it moves every caller
    // onto the slow per-draw path.
    if (!w.truncated.empty() || std::getenv("STANLI_WA_FORCE_INTERP")) {
      // The graph could not express the whole section; hand the model the
      // per-draw interpreter, seeded with data + transformed data and the
      // emission flags the guard blocks test.
      auto env = lo.td.env();
      for (const char* flag :
           {"emit_transformed_parameters__", "emit_generated_quantities__"}) {
        DataMap::Entry one;
        one.is_int = true;
        one.i = {1};
        one.r = {1.0};
        env[flag] = one;
      }
      w.interp = std::make_shared<WaInterp>(prog, std::move(env));
    }
    cm.write_array = std::move(w);
  }
  if (prog->has_transform_inits) {
    // The inverse parameter transforms. Nothing is interpreted here: the
    // section runs only when a caller actually supplies constrained starting
    // values, so a model nobody inits by name never pays for a bound
    // expression this build cannot evaluate.
    CompiledModel::TransformInits ti;
    std::vector<InitParam> params;
    if (cm.views.size() != cm.unc_params.size()) {
      ti.truncated = "the constrained and free parameter lists disagree (" +
                     std::to_string(cm.views.size()) + " vs " +
                     std::to_string(cm.unc_params.size()) + ")";
    } else {
      for (size_t i = 0; i < cm.views.size(); ++i) {
        const CompiledModel::ParamView& view = cm.views[i];
        const CompiledModel::UncParam& unc = cm.unc_params[i];
        if (view.name != unc.name) {
          ti.truncated =
              "constrained and free parameters are out of order at " +
              view.name;
          break;
        }
        InitParam p;
        p.name = view.name;
        p.dims = view.dims;
        p.constrained_len = view.len;
        p.free_len = unc.len;
        // The leaf is the unit the arena keeps contiguous inside each
        // element of the surrounding array. An innermost matrix is one
        // whatever the transform is -- the arena stores it column-major
        // while a serial init lists it first-index-fastest, and an
        // elementwise transform over an array of matrices has to cross that
        // permutation too. Otherwise only a structured transform has a leaf;
        // an elementwise one treats each value on its own, which for a
        // vector or a plain array is the same enumeration either way.
        p.leaf_rank = view.matrix_storage                  ? 2
                      : is_structured_check(unc.transform) ? 1
                                                           : 0;
        if ((size_t)p.leaf_rank > p.dims.size()) {
          ti.truncated = view.name +
                         " declares fewer dimensions than its "
                         "transform needs";
          break;
        }
        params.push_back(std::move(p));
      }
    }
    if (ti.truncated.empty())
      ti.interp =
          std::make_shared<InitInterp>(prog, lo.td.env(), std::move(params));
    cm.transform_inits = std::move(ti);
  }
  prep.plain("compile", "total", compile_time);
  prep.report();
  return cm;
}

}  // namespace stanli
