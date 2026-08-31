// C++ representation of the slice of stanc3's transformed MIR that the graph
// compiler consumes. Anything outside the slice is preserved as raw sexp text
// in `raw` and surfaces as a clear compile error, never a miscompile.
#ifndef STANLI_MIR_HPP
#define STANLI_MIR_HPP

#include <stanli/sexp.hpp>

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace stanli {
namespace mir {

enum class UnsizedLeaf : uint8_t {
  Unknown,
  Int,
  Real,
  Complex,
  Vector,
  RowVector,
  Matrix
};

struct UnsizedView {
  uint8_t depth = 0;
  UnsizedLeaf leaf = UnsizedLeaf::Unknown;
};

struct Expr {
  enum Kind {
    Var,
    LitInt,
    LitReal,
    LitStr,
    FunApp,
    Promotion,
    Indexed,
    TernaryIf,
    EOr,
    EAnd,
    Unsupported
  } kind = Unsupported;
  std::string name;  // Var name or FunApp function name
  enum class Lib { StanLib, Internal, UserDefined } fn_lib = Lib::StanLib;
  bool fn_propto = false;  // (FnLpdf true) / (FnLpmf true)
  long lit_i = 0;
  double lit = 0;
  std::string lit_s;
  std::vector<Expr> args;  // FunApp args; Promotion inner; Indexed base+idx
  std::string type_;       // UInt UReal UVector URowVector UMatrix ...
  UnsizedView unsized;     // structural (UArray ...), without text parsing
  bool data_only = false;  // adlevel DataOnly
  bool promoted = false;   // explicit MIR Promotion to this adlevel/type
  std::string raw;         // Unsupported diagnostics
};

// A matrix row is a non-contiguous Eigen block.  Transposing it changes the
// logical orientation but not the stride, so an outer elementwise expression
// containing it has no packet access and Stan Math's product reduces in
// ascending scalar order.  Both write_array engines consult this syntactic
// fact before materializing the expression, when the stride is still visible.
inline bool is_matrix_row_value(const Expr& value) {
  const Expr* indexed = &value;
  if (value.kind == Expr::FunApp && value.fn_lib == Expr::Lib::StanLib &&
      (value.name == "Transpose__" || value.name == "transpose") &&
      value.args.size() == 1)
    indexed = &value.args[0];
  if (indexed->kind != Expr::Indexed ||
      indexed->unsized.leaf != UnsizedLeaf::RowVector ||
      indexed->args.empty() || indexed->args[0].kind != Expr::Var ||
      indexed->args[0].unsized.depth != 0 ||
      indexed->args[0].unsized.leaf != UnsizedLeaf::Matrix)
    return false;
  const bool implicit_all =
      indexed->args.size() == 2 && indexed->args[1].name == "IndexSingle";
  const bool explicit_all = indexed->args.size() == 3 &&
                            indexed->args[1].name == "IndexSingle" &&
                            indexed->args[2].name == "IndexAll";
  return implicit_all || explicit_all;
}

// A `v[lo:hi]` range slice is `Indexed(v, IndexBetween(lo, hi))`: one
// contiguous span of the same dense, stride-1 storage `v` has, with nothing
// else applied. Peeling it back to the variable lets any classifier that
// already trusts a bare variable's owning-storage provenance trust the slice
// just as much.
inline const Expr& unwrap_range_slice(const Expr& e) {
  if (e.kind == Expr::Indexed && e.args.size() == 2 &&
      e.args[1].kind == Expr::FunApp && e.args[1].name == "IndexBetween")
    return e.args[0];
  return e;
}

enum class ProdGrouping : uint8_t { Legacy, Packet, Scalar };

// Classify only the syntax whose Eigen evaluator provenance has been audited.
// Legacy means "retain MirInterp's old scalar fold / refuse native lowering",
// not a guess about an arbitrary expression's Eigen flags.
inline ProdGrouping prod_grouping(const Expr& product_arg) {
  if (is_matrix_row_value(product_arg)) return ProdGrouping::Scalar;
  if (unwrap_range_slice(product_arg).kind == Expr::Var)
    return ProdGrouping::Packet;
  if (product_arg.kind == Expr::FunApp &&
      product_arg.fn_lib == Expr::Lib::StanLib &&
      (product_arg.name == "Transpose__" || product_arg.name == "transpose") &&
      product_arg.args.size() == 1 && product_arg.args[0].kind == Expr::Var)
    return ProdGrouping::Packet;
  if (product_arg.kind != Expr::FunApp ||
      product_arg.fn_lib != Expr::Lib::StanLib ||
      product_arg.name != "Minus__" || product_arg.args.size() != 2)
    return ProdGrouping::Legacy;

  bool scalar = false;
  for (const Expr& operand : product_arg.args) {
    if (is_matrix_row_value(operand)) {
      scalar = true;
      continue;
    }
    if (operand.unsized.depth == 0 &&
        (operand.kind == Expr::LitInt || operand.kind == Expr::LitReal))
      continue;
    if (operand.kind == Expr::Var) continue;
    if (operand.kind == Expr::FunApp && operand.fn_lib == Expr::Lib::StanLib &&
        (operand.name == "Transpose__" || operand.name == "transpose") &&
        operand.args.size() == 1 && operand.args[0].kind == Expr::Var)
      continue;
    if (operand.kind == Expr::FunApp && operand.fn_lib == Expr::Lib::StanLib &&
        operand.name == "rep_vector" && operand.args.size() == 2 &&
        (operand.args[0].kind == Expr::LitInt ||
         operand.args[0].kind == Expr::LitReal))
      continue;
    return ProdGrouping::Legacy;
  }
  return scalar ? ProdGrouping::Scalar : ProdGrouping::Packet;
}

// One-argument min/max is overloaded across scalars, arrays, matrices, and
// Eigen expressions.  A named Eigen vector, or a contiguous `v[lo:hi]` range
// slice of one, has the owning-storage evaluator provenance audited by the
// generated-quantities extrema opcode: both are a genuine sub-span of the
// same dense storage, so the address-independent grouping the opcode uses
// for a bare variable applies to the slice unchanged. Keeping the function
// kind in the classifier makes every excluded or malformed call an explicit
// Legacy result instead of inferring semantics from a loosely typed
// argument.
enum class ExtremaKind : uint8_t { Legacy, Min, Max };

inline ExtremaKind extrema_kind(const Expr& call) {
  if (call.kind != Expr::FunApp || call.fn_lib != Expr::Lib::StanLib ||
      call.args.size() != 1 || call.type_ != "UReal" ||
      call.unsized.leaf != UnsizedLeaf::Real || call.unsized.depth != 0)
    return ExtremaKind::Legacy;
  const Expr* base = &unwrap_range_slice(call.args[0]);
  const bool vector_arg = base->kind == Expr::Var && base->type_ == "UVector" &&
                          base->unsized.leaf == UnsizedLeaf::Vector &&
                          base->unsized.depth == 0;
  const bool row_vector_arg =
      base->kind == Expr::Var && base->type_ == "URowVector" &&
      base->unsized.leaf == UnsizedLeaf::RowVector && base->unsized.depth == 0;
  if (!vector_arg && !row_vector_arg) return ExtremaKind::Legacy;
  if (call.name == "min") return ExtremaKind::Min;
  if (call.name == "max") return ExtremaKind::Max;
  return ExtremaKind::Legacy;
}

struct Transform {
  // The names are stanc3's own MIR tags, so a new transform in the
  // compiler is greppable here.
  enum Kind {
    Identity,
    Lower,
    Upper,
    LowerUpper,
    Offset,
    Multiplier,
    OffsetMultiplier,
    Simplex,
    Ordered,
    PositiveOrdered,
    CholeskyCorr,
    UnitVector,
    SumToZero,
    Correlation,
    Covariance,
    CholeskyCov,
    Unsupported
  } kind = Identity;
  std::vector<Expr> args;
  std::string raw;
};

inline bool is_structured_check(Transform::Kind kind) {
  switch (kind) {
    case Transform::Simplex:
    case Transform::Ordered:
    case Transform::PositiveOrdered:
    case Transform::CholeskyCorr:
    case Transform::UnitVector:
    case Transform::SumToZero:
    case Transform::Correlation:
    case Transform::Covariance:
    case Transform::CholeskyCov:
      return true;
    default:
      return false;
  }
}

struct SizedType {
  std::string base;        // SInt SReal SVector SRowVector SMatrix SArray ...
  std::vector<Expr> dims;  // outer-to-inner for SArray chains
  std::string elem_base;   // for SArray: the innermost element base
  std::string raw;         // Unsupported diagnostics
  // stanc also uses unsized declarations for optimizer temporaries. Their
  // shape is supplied by the first whole-variable assignment.
  UnsizedView unsized;
};

struct Stmt {
  enum Kind {
    Decl,
    Assignment,
    TargetPE,
    Block,
    SList,
    For,
    IfElse,
    While,
    NRFunApp,
    Return,
    Break,
    Continue,
    Skip,
    Unsupported
  } kind = Unsupported;
  // Decl
  std::string decl_id;
  SizedType decl_type;
  bool decl_data_only = false;
  bool has_init = false;
  Expr init;
  std::optional<Transform> read_transform;  // set iff init is FnReadParam
  std::vector<Expr> read_dims;              // FnReadParam dims
  // Assignment
  std::string lhs;
  std::vector<Expr> lhs_idx;
  Expr rhs;
  // TargetPE
  Expr target;
  // NRFunApp
  std::string fn_name;
  std::vector<Expr> fn_args;
  // FnCheck: the relation lives in the CompilerInternal payload rather than
  // the ordinary argument list. The first fn_arg is the value and the rest
  // are its bounds.
  std::optional<Transform> check_transform;
  std::string check_var_name;
  // FnWriteParam in transform_inits: the transform to INVERT, so that a
  // constrained value supplied by the user becomes a free one. write_array's
  // own FnWriteParam leaves this empty -- its value is constrained already.
  // Both readers split this out of the one optional-transform slot the wire
  // and the S-expression share with FnCheck, so consumers never have to ask
  // which meaning a transform carries.
  std::optional<Transform> write_transform;
  // For
  std::string loopvar;
  Expr lower, upper;
  // IfElse
  Expr cond;
  // Block / SList / For body / IfElse (then at body[0], else at body[1] if
  // present, each wrapped as its own Stmt)
  std::vector<Stmt> body;
  std::string raw;
};

// stanc3 separates write_array's three CSV sections with early-return
// guards rather than nested blocks:
//   if (!(emit_transformed_parameters__ || emit_generated_quantities__))
//     return;                                 <- transformed parameters start
//   if (!emit_generated_quantities__) return; <- generated quantities start
// Both flags are pinned on, so the guards emit nothing; the column count
// as one is reached is the only record of where a section begins.
enum class EmitGuard { None, TransformedParams, GeneratedQuantities };

inline EmitGuard emit_guard(const Stmt& s) {
  if (s.kind != Stmt::IfElse) return EmitGuard::None;
  const Expr& c = s.cond;
  if (c.kind != Expr::FunApp || c.name != "PNot__" || c.args.size() != 1)
    return EmitGuard::None;
  const Expr& a = c.args[0];
  if (a.kind == Expr::Var && a.name == "emit_generated_quantities__")
    return EmitGuard::GeneratedQuantities;
  if (a.kind == Expr::EOr && a.args.size() == 2 &&
      a.args[0].kind == Expr::Var &&
      a.args[0].name == "emit_transformed_parameters__" &&
      a.args[1].kind == Expr::Var &&
      a.args[1].name == "emit_generated_quantities__")
    return EmitGuard::TransformedParams;
  return EmitGuard::None;
}

struct FunDef {
  std::string name;
  std::vector<std::string> arg_names;
  std::vector<std::string> arg_types;  // unsized: UReal UVector UMatrix ...
  std::vector<UnsizedView> arg_views;
  std::vector<bool> arg_data_only;
  std::vector<Stmt> body;
};

struct Program {
  std::vector<std::pair<std::string, SizedType>> input_vars;
  std::vector<Stmt> prepare_data;
  std::vector<Stmt> log_prob;
  // stanc3's `generate_quantities` is the whole write_array body: it re-reads
  // the unconstrained draw, recomputes the transformed parameters, runs the
  // generated quantities block, and marks each CSV column with an
  // FnWriteParam. Gated on emit_transformed_parameters__ /
  // emit_generated_quantities__, which the lowering pins to 1.
  std::vector<Stmt> generate_quantities;
  // stanc3's `transform_inits`: reads each parameter by name from a caller
  // supplied context (FnReadData names the PARAMETER here, not model data)
  // and emits one FnWriteParam per parameter carrying the transform to
  // invert. This is the inverse direction of the log_prob graph's
  // FnReadParam constrains, and the only place stanli can learn it.
  // Presence is separate from content: a current parameterless model carries
  // an explicitly empty section, while an older producer carries no section.
  bool has_transform_inits = false;
  std::vector<Stmt> transform_inits;
  std::vector<FunDef> fun_defs;
  // Output variable names (params, transformed params, generated
  // quantities) in FnWriteParam emission order, from the MIR's
  // output_vars section.
  std::vector<std::string> output_vars;
};

// reduce_sum(f, sliced, grainsize, shared...) reaches its partial-sum
// function through a bare Var rather than a call, so both engines need the
// same three answers about that reference: whether this is a reduce_sum at
// all, which definition the name means, and what propto it asks for.

inline bool is_reduce_sum(const Expr& e) {
  return e.kind == Expr::FunApp && e.fn_lib == Expr::Lib::StanLib &&
         (e.name == "reduce_sum" || e.name == "reduce_sum_static");
}

// The `_lupdf` / `_lupmf` spelling at the functor reference is stanc3's
// propto marker; the definition is always the normalized `_lpdf` / `_lpmf`.
// The marker is the only surviving propto signal here: the functor's
// `(FnLpdf true)` type is not part of the portable encoding, and the
// reduce_sum node itself is FnPlain, so neither can be consulted instead.
inline std::string reduce_sum_partial_name(const std::string& functor,
                                           bool* propto) {
  const auto unnormalized = [&](const char* marker) {
    return functor.size() > 6 &&
           functor.compare(functor.size() - 6, 6, marker) == 0;
  };
  if (unnormalized("_lupdf")) {
    *propto = true;
    return functor.substr(0, functor.size() - 6) + "_lpdf";
  }
  if (unnormalized("_lupmf")) {
    *propto = true;
    return functor.substr(0, functor.size() - 6) + "_lpmf";
  }
  *propto = false;
  return functor;
}

// The formals reduce_sum calls its partial-sum function with, in order: the
// slice, its two bounds, then every shared argument unchanged.
inline std::vector<UnsizedView> reduce_sum_partial_views(const Expr& e) {
  std::vector<UnsizedView> views;
  if (e.args.size() < 3) return views;
  views.reserve(e.args.size());
  views.push_back(e.args[1].unsized);
  views.push_back({0, UnsizedLeaf::Int});
  views.push_back({0, UnsizedLeaf::Int});
  for (size_t i = 3; i < e.args.size(); ++i) views.push_back(e.args[i].unsized);
  return views;
}

// The reader mangles an overloaded definition's name and rewrites its call
// sites, but a functor reference is a Var and is never rewritten. Take the
// unmangled name when it is the only one, and otherwise select the overload
// whose formals match the call reduce_sum will make. Returns null when the
// name resolves to nothing or, impossibly, to more than one.
inline const FunDef* resolve_reduce_sum_partial(
    const std::map<std::string, const FunDef*>& funs, const std::string& base,
    const std::vector<UnsizedView>& views) {
  const auto exact = funs.find(base);
  if (exact != funs.end()) return exact->second;
  const FunDef* match = nullptr;
  for (const auto& [name, def] : funs) {
    if (name.size() <= base.size() || name.compare(0, base.size(), base) != 0 ||
        name[base.size()] != '(')
      continue;
    if (def->arg_views.size() != views.size()) continue;
    bool same = true;
    for (size_t i = 0; i < views.size() && same; ++i)
      same = def->arg_views[i].depth == views[i].depth &&
             def->arg_views[i].leaf == views[i].leaf;
    if (!same) continue;
    if (match) return nullptr;
    match = def;
  }
  return match;
}

Program read_program(const sexp::Node& root);

}  // namespace mir
}  // namespace stanli

#endif
