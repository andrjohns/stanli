#include "lower_internal.hpp"

namespace stanli {
namespace lower_detail {

Lowering::BuiltinDispatch Lowering::resolve_builtin(const mir::Expr& e) {
  if (const auto higher_order = mir::higher_order_call(e)) {
    switch (higher_order->family) {
      case mir::HigherOrderFamily::ReduceSum:
        return {BuiltinFamily::ReduceSum};
      case mir::HigherOrderFamily::MapRect:
        return {BuiltinFamily::MapRect};
      case mir::HigherOrderFamily::Algebra:
        return {BuiltinFamily::Algebra};
      case mir::HigherOrderFamily::Ode:
        return {BuiltinFamily::Ode};
      case mir::HigherOrderFamily::Integrate1D:
        return {BuiltinFamily::Quadrature};
      case mir::HigherOrderFamily::Dae:
        return {BuiltinFamily::Dae};
    }
  }
  // Bespoke functions still own their semantic checks. This registry only
  // selects the handler, replacing the old sequence in which every family
  // was probed and declined in turn.
  static const std::unordered_map<std::string_view, BuiltinDispatch> kBuiltins =
      {
          {"multi_normal_rng", BuiltinFamily::MultiNormalRng},
          {"dirichlet_rng", BuiltinFamily::DirichletRng},
          {"categorical_rng", BuiltinFamily::CategoricalRng},
          {"categorical_logit_rng", BuiltinFamily::CategoricalRng},
          {"append_array", BuiltinFamily::AppendArray},
          {"Transpose__", BuiltinFamily::Matrix},
          {"transpose", BuiltinFamily::Matrix},
          {"tcrossprod", BuiltinFamily::Matrix},
          {"crossprod", BuiltinFamily::Matrix},
          {"diag_pre_multiply", BuiltinFamily::Matrix},
          {"diag_post_multiply", BuiltinFamily::Matrix},
          {"multiply_lower_tri_self_transpose", BuiltinFamily::Matrix},
          {"to_matrix", BuiltinFamily::Matrix},
          {"to_vector", BuiltinFamily::Matrix},
          {"to_row_vector", BuiltinFamily::Matrix},
          {"to_array_1d", BuiltinFamily::Matrix},
          {"rep_matrix", BuiltinFamily::Matrix},
          {"gp_exp_quad_cov", BuiltinFamily::Matrix},
          {"gp_matern32_cov", BuiltinFamily::Matrix},
          {"gp_matern52_cov", BuiltinFamily::Matrix},
          {"gp_exponential_cov", BuiltinFamily::Matrix},
          {"diag_matrix", BuiltinFamily::Matrix},
          {"cholesky_decompose", BuiltinFamily::Matrix},
          {"matrix_exp", BuiltinFamily::Matrix},
          {"inverse", BuiltinFamily::Matrix},
          {"inverse_spd", BuiltinFamily::Matrix},
          {"log_determinant", BuiltinFamily::Matrix},
          {"eigenvalues_sym", BuiltinFamily::Matrix},
          {"eigenvectors_sym", BuiltinFamily::Matrix},
          {"quad_form_diag", BuiltinFamily::Matrix},
          {"quad_form_sym", BuiltinFamily::Matrix},
          {"quad_form", BuiltinFamily::Matrix},
          {"add_diag", BuiltinFamily::Matrix},
          {"append_row", BuiltinFamily::Matrix},
          {"append_col", BuiltinFamily::Matrix},
          {"segment", BuiltinFamily::Matrix},
          {"sub_col", BuiltinFamily::Matrix},
          {"block", BuiltinFamily::Matrix},
          {"col", BuiltinFamily::Matrix},
          {"diagonal", BuiltinFamily::Matrix},
          {"row", BuiltinFamily::Matrix},
          {"head", BuiltinFamily::Matrix},
          {"tail", BuiltinFamily::Matrix},
          {"reverse", BuiltinFamily::Matrix},
          {"rows", BuiltinFamily::ShapeQuery},
          {"cols", BuiltinFamily::ShapeQuery},
          {"size", BuiltinFamily::ShapeQuery},
          {"num_elements", BuiltinFamily::ShapeQuery},

      };
  const auto builtin = kBuiltins.find(e.name);
  if (builtin != kBuiltins.end()) return builtin->second;
  if (const auto regular = resolve_regular_builtin(e.name, e.args.size()))
    return {BuiltinFamily::Elementwise, *regular};
  // Keep the scalar-RNG vocabulary in the shared classifier used by the
  // graph, interpreter, and runtime-region compiler. The selected family is
  // still carried into the handler, so dispatch performs this lookup once.
  if (const ScalarRng* rng = scalar_rng_family(e.name))
    return rng_dispatch(*rng);
  if (ends_with(e.name, "_lpdf") || ends_with(e.name, "_lpmf") ||
      ends_with(e.name, "_cdf") || ends_with(e.name, "_ccdf") ||
      ends_with(e.name, "_lcdf") || ends_with(e.name, "_lccdf"))
    return {BuiltinFamily::Density};
  CallableTransformSpec transform;
  if (callable_transform(e.name, &transform))
    return {BuiltinFamily::CallableTransform};
  return {};
}
// Fallback for expressions with no native lowering: a data-only subtree
// is evaluated at compile time and materialized as a constant. Unsupported
// expressions and Stan validation failures decline; the latter must stay
// at model evaluation rather than move to construction. Propto densities
// never fold because their value is instantiation-dependent.
bool Lowering::expr_effectful(const mir::Expr& e) {
  if (mir::stateful_intrinsic_kind(e)) return true;
  if (e.kind == mir::Expr::FunApp && e.name.size() >= 4 &&
      e.name.compare(e.name.size() - 4, 4, "_rng") == 0)
    return true;
  if (e.kind == mir::Expr::FunApp && e.fn_lib == mir::Expr::Lib::UserDefined &&
      fun_effectful(e.name))
    return true;
  // reduce_sum reaches its partial-sum function through a Var, so the
  // UserDefined test above cannot see a print or reject in that body.
  if (mir::is_reduce_sum(e) && reduce_sum_effectful(e)) return true;
  for (const auto& a : e.args)
    if (expr_effectful(a)) return true;
  return false;
}
bool Lowering::fun_effectful(const std::string& name) {
  auto memo = effectful_cache.find(name);
  if (memo != effectful_cache.end()) return memo->second;

  // Recursion is not itself an observable effect.  Walk the complete
  // reachable call graph for this query, treating an edge back into the
  // active component as already being examined.  Do not memoize an
  // intermediate node: in an effectful recursive component its answer can
  // depend on statements that the outer frame has not visited yet.
  std::set<std::string> visiting;
  std::function<bool(const std::string&)> visit_fun;
  std::function<bool(const mir::Expr&)> visit_expr;
  std::function<bool(const mir::Stmt&)> visit_stmt;

  visit_fun = [&](const std::string& called) {
    auto known = effectful_cache.find(called);
    if (known != effectful_cache.end()) return known->second;
    if (!visiting.insert(called).second) return false;
    bool found = false;
    auto f = fun_defs.find(called);
    if (f != fun_defs.end())
      for (const auto& s : f->second->body)
        if (visit_stmt(s)) {
          found = true;
          break;
        }
    visiting.erase(called);
    return found;
  };

  visit_expr = [&](const mir::Expr& e) {
    if (e.kind == mir::Expr::FunApp && e.name.size() >= 4 &&
        e.name.compare(e.name.size() - 4, 4, "_rng") == 0)
      return true;
    if (e.kind == mir::Expr::FunApp &&
        e.fn_lib == mir::Expr::Lib::UserDefined && visit_fun(e.name))
      return true;
    if (mir::is_reduce_sum(e)) {
      if (e.args.empty() || e.args[0].kind != mir::Expr::Var) return true;
      bool propto = false;
      const mir::FunDef* partial = mir::resolve_callback(
          fun_defs, mir::reduce_sum_partial_name(e.args[0].name, &propto),
          mir::reduce_sum_partial_views(e));
      if (partial == nullptr || visit_fun(partial->name)) return true;
    }
    for (const auto& a : e.args)
      if (visit_expr(a)) return true;
    return false;
  };

  visit_stmt = [&](const mir::Stmt& s) {
    if (s.kind == mir::Stmt::NRFunApp && message_action(s.fn_name)) return true;
    for (const auto& e : s.fn_args)
      if (visit_expr(e)) return true;
    if (s.has_init && visit_expr(s.init)) return true;
    if (visit_expr(s.rhs) || visit_expr(s.target) || visit_expr(s.lower) ||
        visit_expr(s.upper) || visit_expr(s.cond))
      return true;
    for (const auto& e : s.lhs_idx)
      if (visit_expr(e)) return true;
    for (const auto& child : s.body)
      if (visit_stmt(child)) return true;
    return false;
  };

  const bool effect = visit_fun(name);
  effectful_cache[name] = effect;
  return effect;
}
// Integer argument of a density/pmf: values must be known at compile
// time (int data, loop variables, or compile-time expressions).
std::vector<int> Lowering::int_arg_values(LoweredArgument& actual) {
  const mir::Expr& oc = actual.expr();
  if (oc.kind == mir::Expr::Var) {
    DataMap::Entry* en = td.find(oc.name);
    if (en && en->is_int && !en->i.empty()) return en->i;
    if (int_env.count(oc.name)) return {static_cast<int>(int_env[oc.name])};
  }
  if (oc.kind == mir::Expr::LitInt) return {static_cast<int>(oc.lit_i)};
  if (oc.kind == mir::Expr::Indexed) {
    // May be a slice (y[i] on a 2-D array yields a whole row), so
    // evaluate through the data interpreter, not scalar eval_int.
    DataMap::Entry v = eval_pure(oc, "an integer density argument");
    if (v.is_int && !v.i.empty()) return v.i;
  }
  if (oc.kind == mir::Expr::FunApp) {
    // Compile-time int expression (e.g. sum(y[n]) under an unrolled loop).
    return {static_cast<int>(eval_int(oc))};
  }
  fail("int argument must be int data (kind=" + std::to_string((int)oc.kind) +
           " type=" + oc.type_ + ")",
       oc.raw);
}
// Stan's bound transforms, callable as ordinary functions rather than
// written on a declaration. `<t>_constrain(x, bounds...)` is the value
// half of the declaration transform, `<t>_jacobian(...)` is the same
// value and also adds the transform's log absolute jacobian determinant
// to the target, and `<t>_unconstrain(y, bounds...)` is the inverse.
//
// stanc3 marks the jacobian direction with an FnJacobian suffix and emits
// no separate target statement for it, so the increment has to come from
// here -- and only in log_prob, because the generated model instantiates
// write_array with `jacobian__ = false`, which drops it.
//
// Argument 0 always carries the result's shape: every signature in the
// library pairs it either with scalar bounds or with bounds of exactly
// its own type, and none of them widens a scalar first argument against a
// container bound.
std::optional<Lowering::Val> Lowering::lower_callable_transform(
    const mir::Expr& e, CallArguments& actuals) {
  CallableTransformSpec tr;
  if (!callable_transform(e.name, &tr)) return std::nullopt;
  actuals.require_arity(tr.arity);

  if (tr.structured) {
    // The inverse structured transforms are not needed by Jacobian calls
    // and do not share the constrain kernels' two-output protocol.
    if (tr.direction == TransformDirection::Unconstrain) return std::nullopt;
    Val raw = actuals.at(0).value();
    ViewKind leaf = raw.si.kind;
    std::vector<int64_t> dims;
    if (is_array(raw.si)) {
      const ArrayShape& a = array_shape(raw.si);
      dims = a.dims;
      leaf = a.leaf;
    } else if (is_matrix(raw.si)) {
      dims = {raw.si.rows, raw.si.cols};
    } else if (is_vector(raw.si) || is_row_vector(raw.si)) {
      dims = {g.slots[raw.slot].len};
    }
    const size_t rank = (size_t)leaf_rank(leaf);
    if (rank == 0 || dims.size() < rank)
      fail(e.name + ": first argument has an invalid container type", e.raw);
    const size_t outer_rank = dims.size() - rank;
    std::vector<int64_t> outer(dims.begin(), dims.begin() + outer_rank);
    const int64_t batch = checked_product(outer, e.name + " batch");
    int64_t raw_rows = leaf == ViewKind::Matrix ? dims[dims.size() - 2] : 0;
    int64_t raw_cols = leaf == ViewKind::Matrix ? dims.back() : 0;
    int64_t out_rows = 0, out_cols = 0;
    ViewKind out_leaf = leaf;
    uint16_t opcode = tr.opcode;

    switch (tr.kind) {
      case CallableTransformKind::Ordered:
      case CallableTransformKind::PositiveOrdered:
        if (leaf != ViewKind::Vector) fail(e.name + ": expected vector", e.raw);
        out_rows = dims.back();
        break;
      case CallableTransformKind::Simplex:
        if (leaf != ViewKind::Vector) fail(e.name + ": expected vector", e.raw);
        out_rows = dims.back() + 1;
        break;
      case CallableTransformKind::UnitVector:
        if (leaf != ViewKind::Vector) fail(e.name + ": expected vector", e.raw);
        out_rows = dims.back();
        break;
      case CallableTransformKind::SumToZero:
        if (leaf == ViewKind::Vector) {
          out_rows = dims.back() + 1;
        } else if (leaf == ViewKind::Matrix) {
          out_rows = raw_rows + 1;
          out_cols = raw_cols + 1;
          opcode = OP_CONSTRAIN_SUM_TO_ZERO_MAT;
        } else {
          fail(e.name + ": expected vector or matrix", e.raw);
        }
        break;
      case CallableTransformKind::StochasticColumn:
      case CallableTransformKind::StochasticRow:
        if (leaf != ViewKind::Matrix) fail(e.name + ": expected matrix", e.raw);
        out_rows =
            raw_rows + (tr.kind == CallableTransformKind::StochasticColumn);
        out_cols = raw_cols + (tr.kind == CallableTransformKind::StochasticRow);
        break;
      case CallableTransformKind::CholeskyFactorCorr:
      case CallableTransformKind::CorrMatrix:
      case CallableTransformKind::CovMatrix: {
        if (leaf != ViewKind::Vector) fail(e.name + ": expected vector", e.raw);
        const int64_t k =
            actuals.at(1).require_constant_int("matrix dimension");
        out_leaf = ViewKind::Matrix;
        out_rows = out_cols = k;
        break;
      }
      case CallableTransformKind::CholeskyFactorCov:
        if (leaf != ViewKind::Vector) fail(e.name + ": expected vector", e.raw);
        out_leaf = ViewKind::Matrix;
        out_rows = actuals.at(1).require_constant_int("matrix rows");
        out_cols = actuals.at(2).require_constant_int("matrix columns");
        break;
      default:
        fail(e.name + ": invalid structured transform", e.raw);
    }
    if (out_rows < 0 || out_cols < 0)
      fail(e.name + ": negative result dimension", e.raw);
    const int64_t inner_raw =
        leaf == ViewKind::Matrix
            ? checked_product({raw_rows, raw_cols}, e.name + " raw matrix")
            : dims.back();
    const int64_t inner_con =
        out_leaf == ViewKind::Matrix
            ? checked_product({out_rows, out_cols}, e.name)
            : out_rows;
    const int64_t out_len = checked_product({batch, inner_con}, e.name);
    SlotInfo si;
    if (outer_rank != 0) {
      outer.push_back(out_rows);
      if (out_leaf == ViewKind::Matrix) outer.push_back(out_cols);
      si = array_view(std::move(outer), out_leaf, raw.si.param_free);
    } else if (out_leaf == ViewKind::Matrix) {
      si = matrix_view(out_rows, out_cols, raw.si.param_free);
    } else {
      si = view_of(out_leaf == ViewKind::RowVector ? "URowVector" : "UVector");
      si.param_free = raw.si.param_free;
    }
    std::vector<int> idata = {
        checked_immediate(batch, e.name + " batch"),
        checked_immediate(inner_raw, e.name + " raw leaf"),
        checked_immediate(out_leaf == ViewKind::Matrix ? out_rows : inner_con,
                          e.name + " result rows")};
    if (out_leaf == ViewKind::Matrix)
      idata.push_back(checked_immediate(out_cols, e.name + " result columns"));
    const int jac = add_slot(1, false);
    Val v = emit_raw(opcode, {raw.slot}, out_len, si, std::move(idata), jac,
                     raw.autodiff);
    v.layout = owning_layout(si);
    if (tr.direction == TransformDirection::Jacobian && !in_write_array)
      target_terms.push_back(jac);
    return v;
  }

  std::vector<Val> a;
  a.reserve(actuals.size());
  for (size_t i = 0; i < actuals.size(); ++i)
    a.push_back(actuals.at(i).value());
  const int64_t n = g.slots[a[0].slot].len;
  SlotInfo si = a[0].si;
  std::vector<int> ins;
  bool autodiff = false;
  for (const Val& v : a) {
    const int64_t len = g.slots[v.slot].len;
    if (len != 1 && len != n)
      fail(e.name + ": bound is neither one value nor one per element", e.raw);
    si.param_free = si.param_free && v.si.param_free;
    autodiff = autodiff || v.autodiff;
    ins.push_back(v.slot);
  }

  if (tr.direction == TransformDirection::Unconstrain)
    return free_transform(tr.opcode, a, si, n);
  // The declaration kernels, unchanged: they carry the arithmetic that was
  // measured against stan-math's rev overloads, which composing exp,
  // inv_logit, and fma out of the elementwise ops would not reproduce.
  // They always write the jacobian, so `_constrain` allocates the output
  // and simply leaves it unrooted -- no term reaches the target, and its
  // adjoint stays zero, which is exactly the no-lp overload's gradient.
  const int jac = add_slot(1, /*is_param=*/false);
  Val v = emit_raw(tr.opcode, ins, n, si, {}, jac, autodiff);
  v.layout = owning_layout(si);
  if (tr.direction == TransformDirection::Jacobian && !in_write_array)
    target_terms.push_back(jac);
  return v;
}
// The inverse transforms. stan-math has no rev overloads for these: its
// `log(y - lb)` is ordinary var arithmetic, which is what these
// elementwise ops emit, so the composition is the reference rather than an
// approximation of it, and no new kernel is needed.
Lowering::Val Lowering::free_transform(uint16_t opcode,
                                       const std::vector<Val>& a, SlotInfo si,
                                       int64_t n) {
  // An intermediate keeps the argument's logical view only when it is as
  // wide as the argument; `ub - lb` on two scalars is one value.
  const auto elt = [&](uint16_t op, const Val& x, const Val& y) {
    const int64_t w = std::max(g.slots[x.slot].len, g.slots[y.slot].len);
    return with_layout(emit_value(op, {x, y}, w, w == n ? si : SlotInfo{}),
                       elementwise_layout({x, y}));
  };
  const auto un = [&](uint16_t op, const Val& x) {
    return with_layout(emit_value(op, {x}, g.slots[x.slot].len, x.si),
                       elementwise_layout({x}));
  };
  switch (opcode) {
    case OP_CONSTRAIN_LOWER:  // lb_free: log(y - lb)
      return un(OP_LOGV, elt(OP_SUB, a[0], a[1]));
    case OP_CONSTRAIN_UPPER:  // ub_free: log(ub - y)
      return un(OP_LOGV, elt(OP_SUB, a[1], a[0]));
    case OP_CONSTRAIN_LU:  // lub_free: logit((y - lb) / (ub - lb))
      return un(OP_LOGIT,
                elt(OP_DIV, elt(OP_SUB, a[0], a[1]), elt(OP_SUB, a[2], a[1])));
    default:  // offset_multiplier_free: (y - mu) / sigma
      return elt(OP_DIV, elt(OP_SUB, a[0], a[1]), a[2]);
  }
}
// Inline a user-defined function at its call site: arguments are lowered
// in the caller's scope, bound under the parameter names in a shadowed
// scope, and the body lowers like any other statements (loops unroll,
// data-only conditions resolve). Return throws the result value out.
Lowering::Val Lowering::lower_call_udf(
    const mir::Expr& e, const std::function<void()>& before_body) {
  auto it = fun_defs.find(e.name);
  if (it == fun_defs.end()) fail("unknown function " + e.name, e.raw);
  const mir::FunDef& f = *it->second;
  CallArguments actuals(*this, e);
  actuals.require_arity(f.arg_names.size());
  struct Binding {
    bool is_int = false;
    long iv = 0;
    Val v{-1, false, {}};
    std::optional<DataMap::Entry> data;
    bool formal_data_only = false;
  };
  std::vector<Binding> binds(actuals.size());
  for (size_t i = 0; i < actuals.size(); ++i) {
    LoweredArgument& actual = actuals.at(i);
    const mir::Expr& a = actual.expr();
    binds[i].formal_data_only =
        i < f.arg_data_only.size() && f.arg_data_only[i];
    if (!region_current && a.data_only && a.type_ == "UInt") {
      try {
        binds[i].iv = actual.require_constant_int("integer argument");
        binds[i].is_int = true;
      } catch (const CompileError&) {
        if (in_write_array || !needs_runtime_value(a)) throw;
      }
    }
    if (!binds[i].is_int) {
      binds[i].v = actual.value();
      if (const DataMap::Entry* en = actual.observation()) binds[i].data = *en;
    }
    if (!in_write_array && binds[i].formal_data_only && !binds[i].is_int &&
        (binds[i].v.autodiff || !binds[i].v.si.param_free))
      fail(e.name + ": data-only argument depends on a parameter", e.raw);
  }
  // Higher-order calls may validate after evaluating all actual arguments
  // but before entering the user body (reduce_sum's grainsize check).
  if (before_body) before_body();
  if (++udf_depth > 64) {
    --udf_depth;
    fail("UDF recursion too deep in " + e.name);
  }
  auto sc_saved = std::move(scope);
  auto region_cells_saved = region_cells;
  const int region_depth_saved = region_control_depth;
  if (region_current) {
    region_cells.clear();
    region_control_depth = 0;
  }
  auto formal_autodiff_saved = std::move(udf_formal_autodiff);
  auto ie_saved = std::move(int_env);
  auto decls_saved = std::move(decls);
  auto il_saved = std::move(int_locals);
  auto env_saved = std::move(td.env());
  scope.clear();
  udf_formal_autodiff.clear();
  int_env.clear();
  decls.clear();
  int_locals.clear();
  td.env().clear();
  Val ret{-1, false, {}};
  bool returned = false;
  const bool propto_saved = propto_ctx;
  const bool autodiff_saved = udf_autodiff_ctx;
  const bool known_static_saved = write_array_known_static;
  write_array_known_static = false;
  propto_ctx = propto_ctx && e.fn_propto;
  udf_autodiff_ctx = false;
  for (size_t i = 0; i < binds.size(); ++i)
    if (!binds[i].is_int && f.arg_views[i].leaf != mir::UnsizedLeaf::Int)
      udf_autodiff_ctx = udf_autodiff_ctx || binds[i].v.autodiff;
  auto restore = [&] {
    propto_ctx = propto_saved;
    udf_autodiff_ctx = autodiff_saved;
    write_array_known_static = known_static_saved;
    scope = std::move(sc_saved);
    region_cells = std::move(region_cells_saved);
    region_control_depth = region_depth_saved;
    udf_formal_autodiff = std::move(formal_autodiff_saved);
    int_env = std::move(ie_saved);
    decls = std::move(decls_saved);
    int_locals = std::move(il_saved);
    td.env() = std::move(env_saved);
    --udf_depth;
  };
  try {
    for (size_t i = 0; i < binds.size(); ++i) {
      const std::string& name = f.arg_names[i];
      // Bind whenever the argument's value is computable at compile time,
      // not just when the MIR flags it DataOnly: a function may take a data
      // array without the `data` qualifier, and its body still asks for
      // shapes and sizes. Parameter expressions simply fail to evaluate.
      if (binds[i].data) {
        DataMap::Entry en = *binds[i].data;
        td.env()[name] = std::move(en);
      }
      if (binds[i].is_int) {
        int_env[name] = binds[i].iv;
      } else {
        scope[name] = binds[i].v;
        udf_formal_autodiff[name] = binds[i].v.autodiff;
        decls[name] = DeclView{g.slots[binds[i].v.slot].len,
                               binds[i].v.autodiff, binds[i].v.si};
      }
    }
    // CmdStan passes the CALLER's propto__ value into a user density.
    for (const auto& st : f.body) lower_stmt(st);
  } catch (LpReturn& r) {
    ret = r.v;
    returned = true;
  } catch (...) {
    restore();
    throw;
  }
  ret.autodiff = e.unsized.leaf != mir::UnsizedLeaf::Int && udf_autodiff_ctx;
  restore();
  if (!returned) fail(e.name + ": no return value on the executed path");
  ret.layout = owning_layout(ret.si);
  return ret;
}
Lowering::Val Lowering::lower_multi_normal_rng(const mir::Expr& e,
                                               CallArguments& actuals) {
  if (!in_write_array)
    fail("multi_normal_rng is supported only in generated quantities", e.raw);
  if (e.args.size() != 2 || e.type_ != "UVector" ||
      e.unsized.leaf != mir::UnsizedLeaf::Vector || e.unsized.depth != 0)
    fail("multi_normal_rng: expected one vector result", e.raw);
  const mir::Expr& location_expr = actuals.at(0).expr();
  const mir::Expr& covariance_expr = actuals.at(1).expr();
  if (location_expr.type_ != "UVector" ||
      location_expr.unsized.leaf != mir::UnsizedLeaf::Vector ||
      location_expr.unsized.depth != 0)
    fail("multi_normal_rng: expected one vector location", e.raw);
  if (covariance_expr.type_ != "UMatrix" ||
      covariance_expr.unsized.leaf != mir::UnsizedLeaf::Matrix ||
      covariance_expr.unsized.depth != 0)
    fail("multi_normal_rng: expected one covariance matrix", e.raw);

  Val location = actuals.at(0).value();
  Val covariance = actuals.at(1).value();
  if (!is_vector(location.si))
    fail("multi_normal_rng: location is not a logical vector", e.raw);
  if (!is_matrix(covariance.si))
    fail("multi_normal_rng: covariance has no known matrix shape", e.raw);
  const int64_t k = g.slots[location.slot].len;
  if (k > std::numeric_limits<int>::max() || covariance.si.rows != k ||
      covariance.si.cols != k ||
      g.slots[covariance.slot].len != checked_product({k, k}, "covariance"))
    fail("multi_normal_rng: covariance shape must match the location", e.raw);

  Val draw = with_layout(emit_value(OP_RNG, {location, covariance}, k,
                                    view_of(e.type_), {static_cast<int>(k)}),
                         ExpressionLayout::direct());
  g.ops.back().variant = kMultiNormalRngVariant;
  draw.si.param_free = false;
  draw.autodiff = false;
  return draw;
}
Lowering::Val Lowering::lower_dirichlet_rng(const mir::Expr& e,
                                            CallArguments& actuals) {
  if (!in_write_array)
    fail("dirichlet_rng is supported only in generated quantities", e.raw);
  if (e.args.size() != 1 || e.type_ != "UVector" ||
      e.unsized.leaf != mir::UnsizedLeaf::Vector || e.unsized.depth != 0)
    fail("dirichlet_rng: expected one vector result", e.raw);
  const mir::Expr& alpha_expr = actuals.at(0).expr();
  if (alpha_expr.type_ != "UVector" ||
      alpha_expr.unsized.leaf != mir::UnsizedLeaf::Vector ||
      alpha_expr.unsized.depth != 0)
    fail("dirichlet_rng: expected one concentration vector", e.raw);

  Val alpha = actuals.at(0).value();
  if (!is_vector(alpha.si))
    fail("dirichlet_rng: argument is not a logical vector", e.raw);
  const int64_t k = g.slots[alpha.slot].len;
  if (k <= 0 || k > std::numeric_limits<int>::max())
    fail("dirichlet_rng: concentration vector must have a positive length",
         e.raw);

  Val draw = with_layout(emit_value(OP_RNG, {alpha}, k, view_of(e.type_)),
                         ExpressionLayout::direct());
  g.ops.back().variant = kDirichletRngVariant;
  draw.si.param_free = false;
  draw.autodiff = false;
  return draw;
}
Lowering::Val Lowering::lower_regular_unary(uint16_t opcode,
                                            const std::string& type_,
                                            const std::string& name,
                                            const std::string& raw, Val a) {
  SlotInfo si = a.si;
  // Shape-preserving unaries keep rows/cols (softmax/cumulative_sum
  // are vector-only, so they never carry one).
  if (opcode != OP_SOFTMAX && opcode != OP_CUMSUM) {
    if (type_ == "UMatrix" && !is_matrix(si))
      fail(name + ": matrix result has unknown logical extents", raw);
    stamp_kind(&si, type_);
  } else {
    si = view_of(type_);
  }
  si.param_free = a.si.param_free;
  const bool packet_supported =
      opcode != OP_SOFTMAX && opcode != OP_LOG_SOFTMAX && opcode != OP_CUMSUM;
  // These functions return freshly allocated Eigen containers. Their
  // result starts at lane zero independently of the input's provenance;
  // the other unary operations are elementwise evaluator expressions.
  const ExpressionLayout layout =
      packet_supported ? elementwise_layout({a}) : owning_layout(si);
  return with_layout(emit_value(opcode, {a}, g.slots[a.slot].len, si), layout);
}
Lowering::Val Lowering::lower_categorical_rng(const mir::Expr& e,
                                              CallArguments& actuals) {
  const bool is_logit = e.name == "categorical_logit_rng";
  if (!in_write_array)
    fail(e.name + " is supported only in generated quantities", e.raw);
  if (e.args.size() != 1 || e.type_ != "UInt" ||
      e.unsized.leaf != mir::UnsizedLeaf::Int || e.unsized.depth != 0)
    fail(e.name + ": expected one scalar int result", e.raw);
  const mir::Expr& probabilities = actuals.at(0).expr();
  if (probabilities.type_ != "UVector" || probabilities.unsized.depth != 0 ||
      probabilities.unsized.leaf != mir::UnsizedLeaf::Vector)
    fail(e.name + ": expected one " + (is_logit ? "logit" : "probability") +
             "-vector argument",
         e.raw);

  Val argument = actuals.at(0).value();
  if (!is_vector(argument.si))
    fail(e.name + ": argument is not a logical vector", e.raw);
  if (is_logit)
    argument =
        lower_regular_unary(OP_SOFTMAX, "UVector", "softmax", e.raw, argument);
  Val draw = with_layout(emit_value(OP_RNG, {argument}, 1, view_of(e.type_)),
                         ExpressionLayout::scalar());
  g.ops.back().variant = kCategoricalRngVariant;
  // A successful call returns a Stan int, but deliberately do not widen
  // this tranche into runtime-sum range reasoning. Survey only needs the
  // scalar value; dynamic integer control and indexing still fail closed.
  draw.si.param_free = false;
  draw.autodiff = false;
  set_int_initialized(draw);
  return draw;
}
Lowering::Val Lowering::lower_scalar_rng(const mir::Expr& e,
                                         CallArguments& actuals,
                                         ScalarRng family) {
  if (!in_write_array)
    fail(e.name + " is supported only in generated quantities", e.raw);
  const size_t arity = scalar_rng_arity(family);
  if (actuals.size() != arity || e.unsized.depth != 0)
    fail(e.name + ": expected scalar result and " + std::to_string(arity) +
             " scalar argument(s)",
         e.raw);
  const mir::UnsizedLeaf result_leaf = scalar_rng_is_int(family)
                                           ? mir::UnsizedLeaf::Int
                                           : mir::UnsizedLeaf::Real;
  if (e.unsized.leaf != result_leaf)
    fail(e.name + ": result type does not match RNG family", e.raw);
  // Unlike the other scalar families, binomial's (and beta_binomial's)
  // first argument is a population count. Valid stanc MIR always marks it
  // UInt; fail closed on malformed hand-authored MIR rather than silently
  // truncating a real in the runtime helper's graph-storage conversion.
  if ((family == ScalarRng::Binomial || family == ScalarRng::BetaBinomial) &&
      actuals.at(0).expr().unsized.leaf != mir::UnsizedLeaf::Int)
    fail(e.name + ": first argument must be int", e.raw);
  std::vector<Val> args;
  args.reserve(arity);
  for (size_t i = 0; i < actuals.size(); ++i) {
    const mir::Expr& arg = actuals.at(i).expr();
    if (arg.unsized.depth != 0)
      fail(e.name + ": container arguments stay on WaInterp", e.raw);
    args.push_back(actuals.at(i).value());
    if (!is_scalar(args.back()))
      fail(e.name + ": container arguments stay on WaInterp", e.raw);
  }
  Val draw = with_layout(
      arity == 1   ? emit_value(OP_RNG, {args[0]}, 1, view_of(e.type_))
      : arity == 2 ? emit_value(OP_RNG, {args[0], args[1]}, 1, view_of(e.type_))
                   : emit_value(OP_RNG, {args[0], args[1], args[2]}, 1,
                                view_of(e.type_)),
      ExpressionLayout::scalar());
  g.ops.back().variant = static_cast<uint8_t>(family);
  // An effect is never a graph constant, even when all distribution
  // parameters are. This also keeps downstream compile-time demands from
  // mistaking a draw for data.
  draw.si.param_free = false;
  draw.autodiff = false;
  if (scalar_rng_is_int(family)) set_int_initialized(draw);
  if (family == ScalarRng::Bernoulli) set_int_range(draw, 0, 1);
  return draw;
}
Lowering::Val Lowering::lower_append_array(const mir::Expr& e,
                                           CallArguments& actuals) {
  actuals.require_arity(2);
  Val a = actuals.at(0).value();
  Val b = actuals.at(1).value();
  if (!is_array(a.si) || !is_array(b.si))
    fail("append_array: arguments must be arrays", e.raw);
  const ArrayShape& ash = array_shape(a.si);
  const ArrayShape& bsh = array_shape(b.si);
  if (ash.dims.empty() || bsh.dims.empty() ||
      ash.dims.size() != bsh.dims.size() || ash.leaf != bsh.leaf)
    fail("append_array: element shapes must match", e.raw);
  const int64_t a_outer = ash.dims[0], b_outer = bsh.dims[0];
  // stan-math checks element geometry only when both sides contain an
  // element. An empty side contributes no value whose shape could
  // disagree, and the nonempty side supplies the result's suffix.
  if (a_outer != 0 && b_outer != 0 &&
      !std::equal(ash.dims.begin() + 1, ash.dims.end(), bsh.dims.begin() + 1,
                  bsh.dims.end()))
    fail("append_array: element shapes must match", e.raw);
  if (a_outer > std::numeric_limits<int64_t>::max() - b_outer)
    fail("append_array: outer extent overflows", e.raw);
  const int64_t alen = g.slots[a.slot].len;
  const int64_t blen = g.slots[b.slot].len;
  if (alen > std::numeric_limits<int64_t>::max() - blen)
    fail("append_array: storage length overflows", e.raw);
  std::vector<int64_t> dims =
      a_outer == 0 && b_outer != 0 ? bsh.dims : ash.dims;
  dims[0] = a_outer + b_outer;
  const int64_t suffix_count =
      checked_product(std::vector<int64_t>(dims.begin() + 1, dims.end()),
                      "append_array element shape");
  SlotInfo si = array_view(std::move(dims), ash.leaf);
  Val joined = with_layout(emit_value(OP_CONCAT2, {a, b}, alen + blen, si),
                           owning_layout(si));

  // Preserve exact data values for compile-time integer loops and index
  // expressions. Integer arrays are always data-only in Stan, but this
  // also keeps real data arrays available to the ordinary const folder.
  const DataMap::Entry* ao = observation(a);
  const DataMap::Entry* bo = observation(b);
  if (ao && bo && ao->is_int == bo->is_int) {
    DataMap::Entry en;
    en.is_int = ao->is_int;
    en.r.reserve((size_t)(alen + blen));
    // DataMap is first-index-fast, unlike the graph's outer-major array
    // storage. Concatenation along dimension zero therefore interleaves
    // the two outer-axis blocks once for every suffix coordinate.
    const int64_t observation_lanes = a_outer + b_outer == 0 ? 0 : suffix_count;
    for (int64_t lane = 0; lane < observation_lanes; ++lane) {
      const auto ab = ao->r.begin() + lane * a_outer;
      const auto bb = bo->r.begin() + lane * b_outer;
      en.r.insert(en.r.end(), ab, ab + a_outer);
      en.r.insert(en.r.end(), bb, bb + b_outer);
    }
    if (en.is_int) {
      en.i.reserve((size_t)(alen + blen));
      for (int64_t lane = 0; lane < observation_lanes; ++lane) {
        const auto ab = ao->i.begin() + lane * a_outer;
        const auto bb = bo->i.begin() + lane * b_outer;
        en.i.insert(en.i.end(), ab, ab + a_outer);
        en.i.insert(en.i.end(), bb, bb + b_outer);
      }
      set_int_initialized(joined);
      if (!en.i.empty()) {
        const auto bounds = std::minmax_element(en.i.begin(), en.i.end());
        set_int_range(joined, *bounds.first, *bounds.second);
      }
    }
    observe(joined, std::move(en));
  }
  return joined;
}
Lowering::Val Lowering::lower_funapp(const mir::Expr& e) {
  if (const auto intrinsic = mir::stateful_intrinsic_kind(e)) {
    switch (*intrinsic) {
      case mir::StatefulIntrinsicKind::Target: {
        if (in_write_array)
          fail("target() is unavailable in write_array", e.raw);
        SlotInfo si;
        si.param_free = target_terms.empty() && jac_slots.empty();
        return {current_target_slot(), scalar_autodiff(), si};
      }
    }
  }
  if (const auto value = mir::nullary_constant(e)) return constant(*value);
  if (e.fn_lib == mir::Expr::Lib::UserDefined) {
    if (!region_current)
      if (auto v = fold_const(e)) return *v;
    return lower_call_udf(e);
  }
  if (e.fn_lib == mir::Expr::Lib::Internal &&
      (e.name == "FnMakeArray" || e.name == "FnMakeRowVec")) {
    // Array/row-vector literals are structural values: the interpreter's
    // numeric result does not retain enough information to reconstruct an
    // array of containers, so lower the pieces and attach the view here.
    std::vector<Val> parts;
    for (const auto& a : e.args) parts.push_back(lower_expr(a));
    Val acc;
    if (parts.empty()) {
      SlotInfo empty;
      empty.param_free = true;
      acc = Val{add_slot(0, false), false, empty, owning_layout(empty)};
      out.fills.emplace_back(acc.slot, std::vector<double>{});
    } else {
      acc = parts[0];
      for (size_t i = 1; i < parts.size(); ++i) {
        const int64_t len = g.slots[acc.slot].len + g.slots[parts[i].slot].len;
        acc = emit_value(OP_CONCAT2, {acc, parts[i]}, len);
      }
    }
    if (e.name == "FnMakeRowVec") {
      if (e.type_ == "UMatrix") {
        const int64_t rows = (int64_t)parts.size();
        const int64_t cols = parts.empty() ? 0 : g.slots[parts[0].slot].len;
        for (const Val& p : parts)
          if (!is_row_vector(p.si) || g.slots[p.slot].len != cols)
            fail("matrix literal rows have different logical views", e.raw);
        std::vector<int> gather;
        gather.reserve((size_t)(rows * cols));
        for (int64_t j = 0; j < cols; ++j)
          for (int64_t i = 0; i < rows; ++i)
            gather.push_back((int)(i * cols + j));
        acc = emit_value(OP_GATHER, {acc}, rows * cols, matrix_view(rows, cols),
                         gather);
      } else {
        acc.si.kind = ViewKind::RowVector;
        acc.si.shape = 0;
      }
    } else {
      if (parts.empty() && e.unsized.depth != 1)
        fail("empty nested array literal has unknown inner shape", e.raw);
      ViewKind leaf = leaf_kind(e.unsized.leaf);
      std::vector<int64_t> dims{(int64_t)parts.size()};
      if (!parts.empty()) {
        const Val& first = parts.front();
        for (const Val& p : parts)
          if (!same_view(first.si, g.slots[first.slot].len, p.si,
                         g.slots[p.slot].len))
            fail("array literal elements have different logical views", e.raw);
        if (is_array(first.si)) {
          const ArrayShape& child = array_shape(first.si);
          dims.insert(dims.end(), child.dims.begin(), child.dims.end());
          leaf = child.leaf;
        } else if (is_matrix(first.si)) {
          dims.push_back(first.si.rows);
          dims.push_back(first.si.cols);
          leaf = ViewKind::Matrix;
        } else if (is_vector(first.si) || is_row_vector(first.si)) {
          dims.push_back(g.slots[first.slot].len);
          leaf = first.si.kind;
        } else {
          leaf = ViewKind::Flat;
        }
      }
      acc.si = array_view(std::move(dims), leaf, acc.si.param_free);
    }
    if (acc.si.param_free) {
      // MirInterp's scalar-vs-container probe reads child[0], which is not
      // defined for an explicit array of zero-width containers. The view
      // already proves the complete native shape, and an empty value has
      // no bytes to reorder, so record that observation without executing
      // the structurally lossy interpreter path.
      if (g.slots[acc.slot].len == 0) {
        DataMap::Entry en;
        en.is_int = e.unsized.leaf == mir::UnsizedLeaf::Int;
        observe(acc, std::move(en));
      } else if (auto evaluated = try_eval_pure(e)) {
        observe(acc, std::move(*evaluated));
      }
    }
    acc.layout = owning_layout(acc.si);
    return acc;
  }
  if (e.fn_lib != mir::Expr::Lib::StanLib) {
    if (auto v = fold_const(e)) return *v;
    fail("unsupported function kind for " + e.name, e.raw);
  }
  // Construct the lazy argument state exactly once. Resolver and handlers
  // inspect source metadata freely; values are still acquired only when the
  // selected handler asks for them. Nullary constants above need no call
  // state at all.
  CallArguments actuals(*this, e);
  if (e.name == "dims") return lower_dims(e, actuals);
  const BuiltinDispatch dispatch = resolve_builtin(e);

  // One family decision replaces the former chain of optional handlers.
  // A handler can still decline a malformed/unsupported overload so the
  // common constant fallback and diagnostic below remain unchanged.
  switch (dispatch.family) {
    case BuiltinFamily::MapRect:
      if (auto v = lower_empty_map_rect(e, actuals)) return *v;
      return lower_program_expression(e);
    case BuiltinFamily::ReduceSum:
      return lower_reduce_sum(e, actuals);
    case BuiltinFamily::MultiNormalRng:
      return lower_multi_normal_rng(e, actuals);
    case BuiltinFamily::DirichletRng:
      return lower_dirichlet_rng(e, actuals);
    case BuiltinFamily::CategoricalRng:
      return lower_categorical_rng(e, actuals);
    case BuiltinFamily::ScalarRng:
      assert(dispatch.scalar_rng.has_value());
      return lower_scalar_rng(e, actuals, *dispatch.scalar_rng);
    case BuiltinFamily::Density:
      if (auto v = lower_density_fn(e, actuals)) return *v;
      break;
    case BuiltinFamily::CallableTransform:
      if (auto v = lower_callable_transform(e, actuals)) return *v;
      break;
    case BuiltinFamily::Elementwise:
      if (auto v = lower_eltwise_fn(
              e, actuals, dispatch.regular ? &*dispatch.regular : nullptr))
        return *v;
      break;
    case BuiltinFamily::Matrix:
      if (auto v = lower_matrix_fn(e, actuals)) return *v;
      break;
    case BuiltinFamily::Algebra:
      if (const auto call = mir::algebra_call(e.name); call && !call->legacy)
        return lower_program_expression(e);
      return lower_algebra_fn(e, actuals);
    case BuiltinFamily::Quadrature:
      return lower_quadrature_fn(e, actuals);
    case BuiltinFamily::Ode:
      if (const auto call = mir::ode_call(e.name);
          call && call->method == mir::OdeMethod::Adjoint)
        return lower_program_expression(e);
      if (auto v = lower_ode_fn(e, actuals)) return *v;
      break;
    case BuiltinFamily::Dae:
      return lower_program_expression(e);
    case BuiltinFamily::AppendArray:
      if (e.args.size() == 2) return lower_append_array(e, actuals);
      break;
    case BuiltinFamily::ShapeQuery:
      break;
  }
  // A shape query in a REAL-valued expression. eval_int already answers
  // rows/cols/size from the slot or the data map, but only where an
  // integer was expected; brms's mo() helper writes
  // `rows(scale) * sum(scale[1:i])`, where the same call sits in the
  // middle of arithmetic and reached the failure below instead.
  if (dispatch.family == BuiltinFamily::ShapeQuery && e.args.size() == 1) {
    try {
      return constant((double)eval_int(e));
    } catch (const CompileError&) {
    }
  }
  if (auto v = fold_const(e)) return *v;
  fail("unsupported function " + e.name);
}
// Density calls: the table-driven kernels plus exact categorical and
// matrix-argument implementations (multi_normal, lkj, glm).
Lowering::Val Lowering::emit_categorical(const mir::Expr& e, const Val& outcome,
                                         const Val& arg, bool logit) {
  const bool scalar_outcome = e.args[0].unsized.depth == 0;
  if (e.args[0].unsized.leaf != mir::UnsizedLeaf::Int ||
      e.args[0].unsized.depth > 1 ||
      e.args[1].unsized.leaf != mir::UnsizedLeaf::Vector ||
      e.args[1].unsized.depth != 0)
    fail(e.name + ": expected int or array[] int and vector", e.raw);
  const bool array_outcome = is_array(outcome.si) &&
                             array_shape(outcome.si).leaf == ViewKind::Flat &&
                             array_shape(outcome.si).dims.size() == 1;
  if ((scalar_outcome && !is_scalar(outcome)) ||
      (!scalar_outcome && !array_outcome) || !is_vector(arg.si))
    fail(e.name + ": MIR type does not match lowered values", e.raw);
  if (!e.args[0].data_only || !outcome.si.param_free ||
      (udf_depth == 0 &&
       arg.autodiff != (!in_write_array && !e.args[1].data_only)))
    fail(e.name + ": MIR adlevel contradicts lowered dependencies", e.raw);
  auto spec = std::make_shared<CategoricalSpec>();
  spec->logit = logit;
  spec->scalar_outcome = scalar_outcome;
  // The graph dependency and instantiated C++ scalar type are independent:
  // write_array varies with q but uses double, while an autodiff local can
  // be graph-constant and still make Stan retain a propto summand.
  spec->arg_autodiff = arg.autodiff;
  spec->propto = propto(e);
  Val checked = with_layout(emit_value(OP_CATEGORICAL, {outcome, arg}, 1, {}),
                            ExpressionLayout::scalar());
  g.ops.back().udata = spec.get();
  g.udata_pool.push_back(std::move(spec));
  return checked;
}
std::optional<Lowering::Val> Lowering::lower_density_fn(
    const mir::Expr& e, CallArguments& actuals) {
  // Leading integer arguments become idata; the rest become real slots.
  // Layouts: one integer group = raw values; two groups =
  // [len, vals..., len, vals...]; glm = [y..., rows, cols].
  enum class DensityShape {
    Plain,
    FirstMatrixRows,
    FirstMatrixDimensions,
    LastMatrixRowsAndRepetitions,
  };
  struct DensitySpec {
    uint16_t opcode;
    int arity;
    int integer_args;
    bool glm_layout = false;
    DensityShape shape = DensityShape::Plain;
    int activity_mask = -1;  // negative: derive from MIR arguments
    // The single integer group is one outcome per lane of the vectorized
    // reduction, so a language-level scalar broadcasts across the real
    // arguments (see the expansion below). False for the densities whose
    // integer group means something else: multinomial's outcome is the
    // whole count vector, and the ordinal pair reads a cutpoint vector
    // that is one argument rather than lanes.
    bool lane_outcome = false;
  };
  static const std::map<std::string, DensitySpec> kDensities = {
      {"poisson_log_lpmf",
       {OP_POISSON_LOG_LPMF, 2, 1, false, DensityShape::Plain, -1, true}},
      {"bernoulli_logit_lpmf",
       {OP_BERNOULLI_LOGIT_LPMF, 2, 1, false, DensityShape::Plain, -1, true}},

  // clang-format off
        // These macros use the same lists that define opcodes and kernels.
#define STANLI_DENSITY_TABLE(code, fn, n, m) {#fn, {code, n, 0}},
        STANLI_SCALAR_DENSITY_LIST(STANLI_DENSITY_TABLE)
#undef STANLI_DENSITY_TABLE

        // Discrete densities: outcome + n real arguments, one int group,
        // one outcome per lane.
#define STANLI_INT_DENSITY_TABLE(code, fn, nreal, t) \
  {#fn, {code, nreal + 1, 1, false, DensityShape::Plain, -1, true}},
        STANLI_INT_DENSITY_LIST(STANLI_INT_DENSITY_TABLE)
#undef STANLI_INT_DENSITY_TABLE

        // Continuous cdf/lcdf/lccdf functions have no integer group.
#define STANLI_CDF_TABLE(code, fn, n, t) {#fn, {code, n, 0}},
        STANLI_SCALAR_CDF_LIST(STANLI_CDF_TABLE)
#undef STANLI_CDF_TABLE

        // Integer-outcome cdfs keep the count in the one integer group, and
        // it is per-lane there too: a vectorized cdf is the product over
        // lanes, an lcdf/lccdf the sum.
#define STANLI_INT_CDF_TABLE(code, fn, nreal, t) \
  {#fn, {code, nreal + 1, 1, false, DensityShape::Plain, -1, true}},
        STANLI_INT_CDF_LIST(STANLI_INT_CDF_TABLE)
#undef STANLI_INT_CDF_TABLE
        // The binomials' cdfs: an outcome group and a trials group, so
        // the two-group branch below writes both as [len, vals...] and
        // spells a language-level scalar -1. lane_outcome stays false --
        // that flag replicates the ONE group these do not have, and the
        // -1 length is how these broadcast instead.
#define STANLI_TWO_INT_CDF_TABLE(code, fn, nreal, t) {#fn, {code, nreal + 2, 2}},
        STANLI_TWO_INT_CDF_LIST(STANLI_TWO_INT_CDF_TABLE)
#undef STANLI_TWO_INT_CDF_TABLE
        // The var-tape cdfs. Same argument shapes as the two lists above,
        // and a fixed all-active mask because their kernel binds every
        // argument as var whatever the MIR says: one instantiation, no
        // activity-mask expansion, and a data argument's partials
        // computed and dropped.
#define STANLI_TAIL_CDF_TABLE(code, fn, n, t) \
  {#fn, {code, n, 0, false, DensityShape::Plain, (1 << n) - 1}},
        STANLI_TAIL_CDF_LIST(STANLI_TAIL_CDF_TABLE)
#undef STANLI_TAIL_CDF_TABLE
#define STANLI_TAIL_INT_CDF_TABLE(code, fn, nreal, t)                        \
  {#fn,                                                                      \
   {code, nreal + 1, 1, false, DensityShape::Plain, (1 << nreal) - 1, true}},
        STANLI_TAIL_INT_CDF_LIST(STANLI_TAIL_INT_CDF_TABLE)
#undef STANLI_TAIL_INT_CDF_TABLE
        // The ordinal densities have the same argument counts but not the
        // same meaning: their trailing cutpoint vector is one argument, so
        // a scalar outcome stays one lane whatever its length.
#define STANLI_ORDERED_TABLE(code, fn, nreal, t) {#fn, {code, nreal + 1, 1}},
        STANLI_ORDERED_DENSITY_LIST(STANLI_ORDERED_TABLE)
#undef STANLI_ORDERED_TABLE
      // clang-format on

      {"bernoulli_lpmf",
       {OP_BERNOULLI_LPMF, 2, 1, false, DensityShape::Plain, -1, true}},
      {"poisson_lpmf",
       {OP_POISSON_LPMF, 2, 1, false, DensityShape::Plain, -1, true}},
      {"neg_binomial_2_lpmf",
       {OP_NEG_BINOMIAL_2_LPMF, 3, 1, false, DensityShape::Plain, -1, true}},
      {"binomial_lpmf", {OP_BINOMIAL_LPMF, 3, 2}},
      {"binomial_logit_lpmf", {OP_BINOMIAL_LOGIT_LPMF, 3, 2}},
      {"poisson_log_glm_lpmf", {OP_POISSON_LOG_GLM_LPMF, 4, 1, true}},
      {"neg_binomial_2_log_glm_lpmf",
       {OP_NEG_BINOMIAL_2_LOG_GLM_LPMF, 5, 1, true}},
      {"beta_binomial_lpmf", {OP_BETA_BINOMIAL_LPMF, 4, 2}},
      {"bernoulli_logit_glm_lpmf", {OP_BERNOULLI_LOGIT_GLM_LPMF, 4, 1, true}},
      {"dirichlet_lpdf", {OP_DIRICHLET_LPDF, 2, 0}},
      {"multi_normal_cholesky_lpdf",
       {OP_MULTI_NORMAL_CHOL_LPDF, 3, 0, false,
        DensityShape::LastMatrixRowsAndRepetitions}},
      {"multi_normal_lpdf",
       {OP_MULTI_NORMAL_LPDF, 3, 0, false,
        DensityShape::LastMatrixRowsAndRepetitions}},
      {"multi_normal_prec_lpdf",
       {OP_MULTI_NORMAL_PREC_LPDF, 3, 0, false,
        DensityShape::LastMatrixRowsAndRepetitions}},
      {"lkj_corr_cholesky_lpdf",
       {OP_LKJ_CORR_CHOL_LPDF, 2, 0, false, DensityShape::FirstMatrixRows,
        0x1}},
      {"lkj_corr_lpdf",
       {OP_LKJ_CORR_LPDF, 2, 0, false, DensityShape::FirstMatrixRows, 0x1}},
      {"lkj_cov_lpdf",
       {OP_LKJ_COV_LPDF, 4, 0, false, DensityShape::FirstMatrixRows, 0xf}},
      {"multi_gp_lpdf",
       {OP_MULTI_GP_LPDF, 3, 0, false, DensityShape::FirstMatrixDimensions,
        0x7}},
      {"multi_gp_cholesky_lpdf",
       {OP_MULTI_GP_CHOL_LPDF, 3, 0, false, DensityShape::FirstMatrixDimensions,
        0x7}},
      {"multi_student_t_lpdf",
       {OP_MULTI_STUDENT_T_LPDF, 4, 0, false,
        DensityShape::LastMatrixRowsAndRepetitions, 0xf}},
      {"multi_student_t_cholesky_lpdf",
       {OP_MULTI_STUDENT_T_CHOL_LPDF, 4, 0, false,
        DensityShape::LastMatrixRowsAndRepetitions, 0xf}},
      {"multinomial_lpmf",
       {OP_MULTINOMIAL_LPMF, 2, 1, false, DensityShape::Plain, 0x1}},
      {"multinomial_logit_lpmf",
       {OP_MULTINOMIAL_LOGIT_LPMF, 2, 1, false, DensityShape::Plain, 0x1}},
      {"dirichlet_multinomial_lpmf",
       {OP_DIRICHLET_MULTINOMIAL_LPMF, 2, 1, false, DensityShape::Plain, 0x1}},
      {"ordered_probit_lpmf",
       {OP_ORDERED_PROBIT_LPMF, 3, 1, false, DensityShape::Plain, 0x3}},
      {"wiener_lpdf", {OP_WIENER_LPDF, 5, 0, false, DensityShape::Plain, 0x1f}},
      {"wishart_lpdf",
       {OP_WISHART_LPDF, 3, 0, false, DensityShape::FirstMatrixRows, 0x7}},
      {"inv_wishart_lpdf",
       {OP_INV_WISHART_LPDF, 3, 0, false, DensityShape::FirstMatrixRows, 0x7}},
      {"wishart_cholesky_lpdf",
       {OP_WISHART_CHOL_LPDF, 3, 0, false, DensityShape::FirstMatrixRows, 0x7}},
      {"inv_wishart_cholesky_lpdf",
       {OP_INV_WISHART_CHOL_LPDF, 3, 0, false, DensityShape::FirstMatrixRows,
        0x7}},
  };
  auto density = kDensities.find(e.name);
  if (density != kDensities.end()) {
    const DensitySpec& spec = density->second;
    if ((int)actuals.size() != spec.arity) {
      // The compact cases below used to decline a bad arity and let the
      // common unsupported-function diagnostic report it.
      if (spec.shape != DensityShape::Plain || spec.activity_mask >= 0)
        return std::nullopt;
      actuals.require_arity((size_t)spec.arity);
    }
    std::vector<int> idata;
    // Whether argument 0 was written as a bare `int` rather than an
    // array. Same test as the two-group path below, and for the same
    // reason: a length-1 array is a container that must match the other
    // arguments' size, a scalar broadcasts.
    bool scalar_outcome = false;
    if (spec.integer_args == 1) {
      idata = int_arg_values(actuals.at(0));
      scalar_outcome =
          actuals.at(0).expr().type_ == "UInt" && idata.size() == 1;
    } else if (spec.integer_args == 2) {
      // Group length -1 marks a language-level scalar (broadcast in
      // stan-math); a length-1 array stays a vector, as CmdStan would
      // instantiate it.
      auto put = [&](LoweredArgument& actual) {
        auto vals = int_arg_values(actual);
        const bool scalar = actual.expr().type_ == "UInt" && vals.size() == 1;
        idata.push_back(scalar ? -1 : (int)vals.size());
        idata.insert(idata.end(), vals.begin(), vals.end());
      };
      put(actuals.at(0));
      put(actuals.at(1));
    }
    std::vector<int> ins;
    SlotInfo shapes[6]{};
    SlotInfo result_si{0, 0, true};
    bool result_autodiff = false;
    uint8_t variant = spec.activity_mask < 0 ? 0 : (uint8_t)spec.activity_mask;
    for (size_t i = spec.integer_args; i < e.args.size(); ++i) {
      const Val arg = actuals.at(i).value();
      shapes[i - spec.integer_args] = arg.si;
      ins.push_back(arg.slot);
      result_si.param_free = result_si.param_free && arg.si.param_free;
      result_autodiff = result_autodiff || arg.autodiff;
      if (spec.activity_mask < 0 && !actuals.at(i).expr().data_only)
        variant |= (uint8_t)(1u << (i - spec.integer_args));
    }
    // A scalar outcome against vectorized real arguments: replicate it to
    // the lane count. The kernels map the whole integer group as one
    // Eigen::VectorXi, so a scalar arrived at stan-math as a size-1
    // container and lost against a longer argument on
    // check_consistent_sizes -- "Failures variable has size = 1, but
    // Number of successes parameter has size 2". Expanding here rather
    // than adding a scalar-bound instantiation to every kernel keeps the
    // graph identical to the one the equivalent array-outcome model
    // produces, which is already the verified shape: each of these is a
    // per-lane reduction (sum for the lpmfs and lcdf/lccdf, product for
    // the cdfs), so N copies of the outcome is the same math in the same
    // order as one broadcast scalar. binomial_logit_glm below does the
    // same thing for the same reason.
    if (spec.lane_outcome && scalar_outcome) {
      int64_t lanes = 1;
      for (int slot : ins) lanes = std::max(lanes, g.slots[slot].len);
      if (lanes > 1) {
        // By value: assign() may reallocate, and a reference into the
        // vector being assigned would dangle mid-fill.
        const int outcome = idata[0];
        idata.assign((size_t)lanes, outcome);
      }
    }
    if (propto(e)) variant |= 0x80u;
    if (spec.glm_layout) {
      // X must be a data matrix; append its dims to idata.
      const SlotInfo& xsi = shapes[0];
      if (!is_matrix(xsi) || !xsi.param_free)
        fail(e.name + ": X must be a data matrix");
      // A GLM's outcome group is one value per ROW of X, and the kernels
      // map exactly that many out of idata -- so a group of any other
      // length is refused here rather than read past the end of the
      // vector. These are the entries left lane_outcome = false, and not
      // by oversight: that expansion sizes itself from the longest real
      // argument, which for a GLM is X at rows*cols.
      //
      // Nor could poisson_log_glm simply replicate to `rows`. stan-math
      // accepts a language-level scalar outcome and broadcasts it, but
      // its <false> form then subtracts lgamma(y+1) ONCE rather than once
      // per row (four rows of y = 3: -4.98, against the -10.36 the
      // replicated array gives, with identical gradients). Replicating
      // would buy the right gradients and an lp a constant off CmdStan's,
      // which is the one thing these kernels exist to get right. The
      // other two were measured and do not have that problem; they share
      // this layout and this check, so they are refused with it.
      if ((int64_t)idata.size() != xsi.rows)
        fail(e.name + ": outcome has " + std::to_string(idata.size()) +
                 " value(s) but X has " + std::to_string(xsi.rows) +
                 " rows; a scalar or short outcome is unsupported",
             e.raw);
      idata.push_back((int)xsi.rows);
      idata.push_back((int)xsi.cols);
    }
    if (spec.shape == DensityShape::FirstMatrixRows) {
      if (!is_matrix(shapes[0])) {
        if (e.name == "lkj_cov_lpdf") fail("lkj_cov needs a matrix", e.raw);
        fail(e.name + " needs a matrix", e.raw);
      }
      idata = {(int)shapes[0].rows};
    } else if (spec.shape == DensityShape::FirstMatrixDimensions) {
      if (!is_matrix(shapes[0]) || !is_matrix(shapes[1]))
        fail(e.name + " needs matrix arguments", e.raw);
      idata = {(int)shapes[0].rows, (int)shapes[0].cols};
    } else if (spec.shape == DensityShape::LastMatrixRowsAndRepetitions) {
      const size_t last = ins.size() - 1;
      if (!is_matrix(shapes[last])) {
        if (e.name.rfind("multi_normal", 0) == 0)
          fail(e.name + ": needs a matrix argument (got length " +
                   std::to_string(g.slots[ins[last]].len) + ")",
               e.raw);
        fail(e.name + " needs a matrix argument", e.raw);
      }
      const int64_t K = shapes[last].rows;
      if (K < 0 || shapes[last].cols != K)
        fail(e.name + ": matrix argument must be square", e.raw);

      // The native kernels accept one vector/row-vector location and a
      // vector or array of vectors on the left. Derive repetitions from
      // the logical view, not a division by K: that remains defined for
      // legal zero-dimensional vectors and catches short flat storage
      // before a kernel can read past it. multi_student_t has nu between
      // y and mu; the multi_normal forms do not.
      const auto vector_repetitions = [&](size_t arg, const char* role) {
        const SlotInfo& si = shapes[arg];
        const int64_t len = g.slots[ins[arg]].len;
        if (is_vector(si) || is_row_vector(si)) {
          if (len != K) {
            if (arg == 0)
              fail(e.name + ": random variable length " + std::to_string(len) +
                       " is not a positive multiple of matrix size " +
                       std::to_string(K),
                   e.raw);
            fail(e.name + ": " + role + " length " + std::to_string(len) +
                     " does not match matrix size " + std::to_string(K),
                 e.raw);
          }
          return int64_t{1};
        }
        if (is_array(si)) {
          const ArrayShape& array = array_shape(si);
          if ((array.leaf != ViewKind::Vector &&
               array.leaf != ViewKind::RowVector) ||
              array.dims.empty() || array.dims.back() != K)
            fail(e.name + ": " + role +
                     " must be a vector or an array of vectors",
                 e.raw);
          std::vector<int64_t> outer(array.dims.begin(), array.dims.end() - 1);
          const int64_t repetitions =
              checked_product(outer, e.name + ": " + role + " shape");
          if (checked_product({repetitions, K},
                              e.name + ": " + role + " storage") != len)
            fail(e.name + ": " + role +
                     " logical shape does not match storage length",
                 e.raw);
          return repetitions;
        }
        fail(e.name + ": " + role + " must be a vector or an array of vectors",
             e.raw);
      };
      const int64_t repetitions = vector_repetitions(0, "random variable");
      if (repetitions == 0)
        fail(e.name + ": an empty array of random variables is unsupported",
             e.raw);
      const size_t location = e.name.rfind("multi_student_t", 0) == 0 ? 2 : 1;
      const int64_t locations = vector_repetitions(location, "location");
      idata = {(int)K, (int)repetitions};
      // Only the multi_normal kernels read an array of locations.
      if (locations != 1) {
        const bool multi_normal = spec.opcode == OP_MULTI_NORMAL_LPDF ||
                                  spec.opcode == OP_MULTI_NORMAL_PREC_LPDF ||
                                  spec.opcode == OP_MULTI_NORMAL_CHOL_LPDF;
        if (!multi_normal)
          fail(e.name + ": an array-valued location is unsupported", e.raw);
        if (repetitions != 1 && locations != repetitions)
          fail(e.name + ": " + std::to_string(repetitions) +
                   " random variables against " + std::to_string(locations) +
                   " locations",
               e.raw);
        idata.push_back((int)locations);
      }
    }
    Val dv =
        emit_raw(spec.opcode, ins, 1, result_si, idata, -1, result_autodiff);
    dv.layout = ExpressionLayout::scalar();
    // GLM ops used to be the one density shape that got no variant at
    // all, so their kernels hardcoded propto=false and poisson_log_glm's
    // lp landed sum(log(y!)) -- 10.45 on a six-row test -- away from
    // CmdStan's, with the gradients already exact. They get the same
    // variant as everything else now. Their forwards bind arguments
    // explicitly rather than through mask_dispatch, so the mask reaches
    // only density_bwd, where it says to skip X -- which it already did
    // by X's null adjoint. Setting only the propto bit is NOT an option:
    // density_bwd reads a nonzero variant as a literal mask, so 0x80
    // alone means "no argument is active" and every gradient comes back
    // zero.
    g.ops.back().variant = variant;
    return dv;
  }

  if (e.name == "categorical_logit_lpmf" && e.args.size() == 2) {
    const Val outcome = actuals.at(0).value();
    Val b = actuals.at(1).value();
    return emit_categorical(e, outcome, b, true);
  }
  if (e.name == "categorical_lpmf" && e.args.size() == 2) {
    const Val outcome = actuals.at(0).value();
    Val th = actuals.at(1).value();
    return emit_categorical(e, outcome, th, false);
  }

  // gaussian_dlm_obs takes seven arguments and Op::in holds six, so it
  // cannot be lowered as one op at all. Raising the limit would add
  // bytes to every Op and every KernelCtx in every model for the sake
  // of one dynamic-linear-model density, so this refuses instead and
  // names the reason. See docs/coverage.md.
  if (e.name == "gaussian_dlm_obs_lpdf")
    fail("gaussian_dlm_obs takes 7 arguments and an op holds 6", e.raw);

  // The three GLMs whose argument shapes the recorder cannot express.
  // idata is the outcome (two groups for binomial) then rows, cols.
  if (e.name == "binomial_logit_glm_lpmf" && e.args.size() == 5) {
    std::vector<int> idata = int_arg_values(actuals.at(0));
    std::vector<int> NN = int_arg_values(actuals.at(1));
    Val X = actuals.at(2).value();
    Val alpha = actuals.at(3).value();
    Val beta = actuals.at(4).value();
    if (!is_matrix(X.si)) fail("binomial_logit_glm needs a matrix", e.raw);
    // Both int groups are one value per row, so a scalar broadcasts.
    // By value, as the lane_outcome expansion above is: assign() may
    // reallocate, and a reference into the vector being assigned would
    // dangle mid-fill.
    const int64_t rows = X.si.rows;
    if ((int64_t)idata.size() == 1) {
      const int outcome = idata[0];
      idata.assign(rows, outcome);
    }
    if ((int64_t)NN.size() == 1) {
      const int trials = NN[0];
      NN.assign(rows, trials);
    }
    idata.insert(idata.end(), NN.begin(), NN.end());
    idata.push_back((int)rows);
    idata.push_back((int)X.si.cols);
    Val v = with_layout(
        emit_value(OP_BINOMIAL_LOGIT_GLM_LPMF, {X, alpha, beta}, 1, {}, idata),
        ExpressionLayout::scalar());
    g.ops.back().variant = (uint8_t)((propto(e) ? 0x80u : 0u) | 0x7u);
    return v;
  }
  if ((e.name == "categorical_logit_glm_lpmf" ||
       e.name == "ordered_logistic_glm_lpmf") &&
      e.args.size() == 4) {
    std::vector<int> idata = int_arg_values(actuals.at(0));
    Val X = actuals.at(1).value();
    Val a2 = actuals.at(2).value();
    Val a3 = actuals.at(3).value();
    if (!is_matrix(X.si)) fail(e.name + " needs a matrix", e.raw);
    if ((int64_t)idata.size() == 1) {
      // By value, as the lane_outcome expansion is: assign() may
      // reallocate, and a reference into the vector being assigned would
      // dangle mid-fill.
      const int outcome = idata[0];
      idata.assign(X.si.rows, outcome);
    }
    idata.push_back((int)X.si.rows);
    idata.push_back((int)X.si.cols);
    // categorical: (y, x, alpha, beta). ordered: (y, x, beta, cuts).
    // Both pass their two real arguments in order, so the kernel reads
    // in[1] and in[2] and knows from its Kind what they mean.
    Val v = with_layout(emit_value(e.name == "categorical_logit_glm_lpmf"
                                       ? OP_CATEGORICAL_LOGIT_GLM_LPMF
                                       : OP_ORDERED_LOGISTIC_GLM_LPMF,
                                   {X, a2, a3}, 1, {}, idata),
                        ExpressionLayout::scalar());
    g.ops.back().variant = (uint8_t)((propto(e) ? 0x80u : 0u) | 0x7u);
    return v;
  }

  if (e.name == "normal_id_glm_lpdf" && e.args.size() == 5) {
    Val y = actuals.at(0).value();
    Val X = actuals.at(1).value();
    if (!is_matrix(X.si)) fail("normal_id_glm: X must be a matrix", e.raw);
    Val alpha = actuals.at(2).value();
    Val beta = actuals.at(3).value();
    Val sigma = actuals.at(4).value();
    uint8_t variant = 0;
    for (int i = 0; i < 5; ++i)
      if (!actuals.at(i).expr().data_only) variant |= (uint8_t)(1u << i);
    if (propto(e)) variant |= 0x80u;
    Val v = with_layout(
        emit_value(OP_NORMAL_ID_GLM_LPDF, {y, X, alpha, beta, sigma}, 1, {},
                   {(int)X.si.rows, (int)X.si.cols}),
        ExpressionLayout::scalar());
    g.ops.back().variant = variant;
    return v;
  }
  return std::nullopt;
}
// Elementwise math, reductions, and dot products.
std::optional<Lowering::Val> Lowering::lower_eltwise_fn(
    const mir::Expr& e, CallArguments& actuals, const RegularSpec* regular) {
  // Once a generated int RNG has become a runtime scalar slot, named
  // integer division is no longer foldable. OP_DIV is real division and
  // would return 3.5 for divide(7, 2), while Stan truncates to 3. Refuse it
  // so the whole write_array stays on WaInterp until there is a native int
  // division op. The operator spelling is IntDivide__ and already refuses.
  if ((e.name == "divide" || e.name == "elt_divide") && e.type_ == "UInt")
    fail(e.name + ": runtime integer division stays on WaInterp", e.raw);
  // `A \ B` and `B / A` with a matrix divisor are linear solves, not
  // elementwise division: stanc spells them with the ordinary division
  // operators and lowers them to mdivide_left/mdivide_right. The divisor's
  // type is the whole discriminator -- a scalar divisor is elementwise, and
  // `./` is never a solve -- which is the rule the MIR interpreter applies,
  // kept identical here so a solve does not mean one thing in the model
  // block and another in transformed data.
  //
  // The named spellings share this lowering: they arrive with the same
  // argument order the operators use, divisor first for a left solve and
  // second for a right one. The _spd and _tri_low families get their own
  // opcodes rather than a flag because stan-math answers them by different
  // factorisations -- an LLT of a symmetric positive definite matrix, and
  // a triangular solve that never reads the upper triangle -- so they are
  // different results, not faster routes to the same one.
  struct NamedSolve {
    const char* name;
    bool left;
    uint16_t opcode;
  };
  static constexpr NamedSolve kNamedSolves[] = {
      {"mdivide_left", true, OP_MDIVIDE_LEFT},
      {"mdivide_right", false, OP_MDIVIDE_RIGHT},
      {"mdivide_left_spd", true, OP_MDIVIDE_LEFT_SPD},
      {"mdivide_right_spd", false, OP_MDIVIDE_RIGHT_SPD},
      {"mdivide_left_tri_low", true, OP_MDIVIDE_LEFT_TRI_LOW},
      {"mdivide_right_tri_low", false, OP_MDIVIDE_RIGHT_TRI_LOW},
  };
  const NamedSolve* named_solve = nullptr;
  if (e.args.size() == 2)
    for (const NamedSolve& candidate : kNamedSolves)
      if (e.name == candidate.name) named_solve = &candidate;
  if (named_solve != nullptr || e.name == "LDivide__" ||
      (e.name == "Divide__" && e.args.at(1).type_ == "UMatrix")) {
    const bool left =
        named_solve != nullptr ? named_solve->left : e.name == "LDivide__";
    const uint16_t opcode = named_solve != nullptr
                                ? named_solve->opcode
                                : (left ? OP_MDIVIDE_LEFT : OP_MDIVIDE_RIGHT);
    actuals.require_arity(2);
    Val a = actuals.at(0).value();
    Val b = actuals.at(1).value();
    const Val& divisor = left ? a : b;
    const Val& dividend = left ? b : a;
    // rows <= 0 is a matrix view whose shape the lowering never resolved;
    // the kernel would map n x n over the slot and read past it.
    if (!is_matrix(divisor.si) || divisor.si.rows != divisor.si.cols ||
        divisor.si.rows <= 0)
      fail(e.name + ": divisor is not a square matrix of known size", e.raw);
    const int64_t n = divisor.si.rows;
    // A non-matrix dividend is the vector its side implies -- a column
    // under `\`, a row under `/` -- the same rule Times__ follows. Either
    // way the shared extent is n and the result has the dividend's shape.
    const bool dm = is_matrix(dividend.si);
    if (!dm && !is_vector(dividend.si) && !is_row_vector(dividend.si))
      fail(e.name + ": dividend is not a matrix or vector", e.raw);
    const int64_t shared = dm ? (left ? dividend.si.rows : dividend.si.cols)
                              : g.slots[dividend.slot].len;
    if (shared != n)
      fail(e.name + ": inner dimension mismatch (" + std::to_string(n) + "x" +
               std::to_string(n) + " against " + std::to_string(shared) + ")",
           e.raw);
    const int64_t k = dm ? (left ? dividend.si.cols : dividend.si.rows) : 1;
    Val v = emit_value(opcode, {a, b}, n * k, dividend.si, {(int)n, (int)k});
    // The kernel solves through the operand types CmdStan's generated code
    // would have used, because stan-math answers differently for each: bit
    // 0 says the result is var, bit 1 says the dividend is a vector rather
    // than a one-column matrix, and bits 2/3 retain the divisor/dividend
    // scalar types so mixed vv/vd/dv overloads do not collapse to vv.
    g.ops.back().variant =
        (uint8_t)((v.autodiff ? 1u : 0u) | (dm ? 0u : 2u) |
                  (divisor.autodiff ? 4u : 0u) | (dividend.autodiff ? 8u : 0u));
    return with_layout(v, owning_layout(dividend.si));
  }
  // multiply is the named spelling of `*`, including its linear algebra:
  // the branches below pick matvec, GEMM, outer and inner products off
  // the operand views and the result type, which the alias shares.
  if (e.name == "Times__" || (e.name == "multiply" && e.args.size() == 2)) {
    actuals.require_arity(2);
    Val a = actuals.at(0).value();
    Val b = actuals.at(1).value();
    // Scalar on either side is an elementwise scale, whatever shape the
    // other operand carries.
    const bool a_scalar = is_scalar(a);
    const bool b_scalar = is_scalar(b);
    if (a_scalar || b_scalar) {
      const Val& shaped = a_scalar ? b : a;
      SlotInfo si = shaped.si;
      si.param_free = a.si.param_free && b.si.param_free;
      const int64_t n = a_scalar ? g.slots[b.slot].len : g.slots[a.slot].len;
      return with_layout(emit_value(OP_MUL, {a, b}, n, si),
                         elementwise_layout({a, b}));
    }
    if (is_matrix(a.si)) {
      if (a.si.param_free && is_vector(b.si)) {
        if (g.slots[b.slot].len != a.si.cols)
          fail(e.name + ": inner dimension mismatch", e.raw);
        return with_layout(
            emit_value(OP_MATVEC, {a, b}, a.si.rows, view_of("UVector"),
                       {(int)a.si.rows, (int)a.si.cols}),
            owning_layout(view_of("UVector")));
      }
      // General product; a vector operand is one column.
      const int64_t cb = is_matrix(b.si) ? b.si.cols : 1;
      const int64_t rb = is_matrix(b.si) ? b.si.rows : g.slots[b.slot].len;
      if (rb != a.si.cols)
        fail(e.name + ": inner dimension mismatch (" +
                 std::to_string(a.si.rows) + "x" + std::to_string(a.si.cols) +
                 " times " + std::to_string(rb) + "x" + std::to_string(cb) +
                 ")",
             e.raw);
      SlotInfo si =
          e.type_ == "UMatrix"
              ? matrix_view(a.si.rows, cb)
              : (cb == 1 ? view_of("UVector") : matrix_view(a.si.rows, cb));
      Val v = emit_value(OP_GEMM, {a, b}, a.si.rows * cb, si,
                         {(int)a.si.rows, (int)a.si.cols, (int)cb});
      return with_layout(v, owning_layout(si));
    }
    // vector * row_vector with a matrix result is an outer product.
    if (is_vector(a.si) && is_row_vector(b.si) && e.type_ == "UMatrix") {
      const int64_t nr = g.slots[a.slot].len, nc = g.slots[b.slot].len;
      SlotInfo si = matrix_view(nr, nc);
      return with_layout(
          emit_value(OP_GEMM, {a, b}, nr * nc, si, {(int)nr, 1, (int)nc}),
          owning_layout(si));
    }
    if (is_row_vector(a.si) && is_matrix(b.si)) {
      const int64_t k = g.slots[a.slot].len;
      if (k != b.si.rows) fail(e.name + ": inner dimension mismatch", e.raw);
      return with_layout(
          emit_value(OP_GEMM, {a, b}, b.si.cols, view_of("URowVector"),
                     {1, (int)k, (int)b.si.cols}),
          owning_layout(view_of("URowVector")));
    }
    // row_vector * vector with scalar result type is an inner product.
    if (is_row_vector(a.si) && is_vector(b.si) &&
        (e.type_ == "UReal" || e.type_ == "UInt")) {
      if (g.slots[a.slot].len != g.slots[b.slot].len)
        fail(e.name + ": inner dimension mismatch", e.raw);
      return with_layout(emit_value(OP_DOT, {a, b}, 1),
                         ExpressionLayout::scalar());
    }
    if (a.si.kind != ViewKind::Flat || b.si.kind != ViewKind::Flat)
      fail(e.name + ": unsupported container product", e.raw);
    const int64_t len = std::max(g.slots[a.slot].len, g.slots[b.slot].len);
    return with_layout(emit_value(OP_MUL, {a, b}, len),
                       elementwise_layout({a, b}));
  }
  // fma from --O1 partial evaluation (`c + a*b`) or written explicitly:
  // fused (std::fma), elementwise with scalar broadcast on any argument.
  if (e.name == "fma" && e.args.size() == 3) {
    Val a = actuals.at(0).value();
    Val b = actuals.at(1).value();
    Val c = actuals.at(2).value();
    const int64_t la = g.slots[a.slot].len, lb = g.slots[b.slot].len,
                  lc = g.slots[c.slot].len;
    const int64_t n = std::max(la, std::max(lb, lc));
    for (int64_t l : {la, lb, lc})
      if (l != n && l != 1) fail("fma: incompatible lengths", e.raw);
    // The shape of whichever operand carries one, like the binaries.
    SlotInfo si = shape_of(a, b);
    if (is_scalar(a) && is_scalar(b)) si = shape_of(a, c);
    si.param_free = a.si.param_free && b.si.param_free && c.si.param_free;
    return with_layout(emit_value(OP_FMA, {a, b, c}, n, si),
                       elementwise_layout({a, b, c}));
  }
  if (regular != nullptr && regular->kind == RegularKind::Binary) {
    actuals.require_arity(2);
    Val a = actuals.at(0).value();
    Val b = actuals.at(1).value();
    const int64_t la = g.slots[a.slot].len, lb = g.slots[b.slot].len;
    const bool as = is_scalar(a), bs = is_scalar(b);
    if (!as && !bs && !same_view(a.si, la, b.si, lb))
      fail(e.name + ": incompatible logical views");
    // Elementwise results keep the matrix shape of whichever operand
    // has one; losing it would make a later Times__ miss the matvec.
    SlotInfo si = shape_of(a, b);
    const int64_t n = as ? lb : (bs ? la : la);
    return with_layout(emit_value(regular->opcode, {a, b}, n, si),
                       elementwise_layout({a, b}));
  }

  if (regular != nullptr && (regular->kind == RegularKind::BinaryIntFirst ||
                             regular->kind == RegularKind::BinaryIntSecond)) {
    return lower_binary_int(
        regular->opcode, regular->kind == RegularKind::BinaryIntFirst, actuals);
  }

  if (regular != nullptr && regular->kind == RegularKind::Unary) {
    actuals.require_arity(1);
    return lower_regular_unary(regular->opcode, e.type_, e.name, e.raw,
                               actuals.at(0).value());
  }
  // plus, and its operator spelling, are the identity on every shape.
  if (e.name == "PPlus__" || (e.name == "plus" && e.args.size() == 1)) {
    actuals.require_arity(1);
    return actuals.at(0).value();
  }
  if (e.name == "min" || e.name == "max") {
    // Preserve the construction-time path for well-formed data-only
    // extrema, including the scalar two-argument overload.  Dynamic
    // lowering now uses the lowered argument's layout rather than
    // re-deriving its provenance from MIR syntax.
    if (e.args.size() == 1 || e.args.size() == 2)
      if (auto v = fold_const(e)) return *v;
    const mir::ExtremaCall call = mir::extrema_call(e);
    if (call.kind == mir::ExtremaKind::Legacy)
      fail("min/max expression surface stays on WaInterp", e.raw);
    return call.surface == mir::ExtremaSurface::IntPair
               ? lower_extrema_pair(e, actuals, call.kind)
               : lower_extrema_reduction(e, actuals, call);
  }
  if (e.name == "mean") {
    actuals.require_arity(1);
    Val a = actuals.at(0).value();
    return with_layout(emit_value(OP_MEAN, {a}, 1), ExpressionLayout::scalar());
  }
  if (e.name == "prod") {
    // Preserve the pre-existing construction-time behavior for data-only
    // products. Dynamic products use OP_PROD_VEC in either graph.
    if (auto v = fold_const(e)) return *v;
    if (e.args.size() != 1 || e.type_ != "UReal" ||
        e.unsized.leaf != mir::UnsizedLeaf::Real || e.unsized.depth != 0)
      fail("prod needs exactly one scalar-real result", e.raw);
    actuals.require_arity(1);
    Val a = actuals.at(0).value();
    if ((!is_vector(a.si) && !is_row_vector(a.si)) || g.slots[a.slot].len <= 0)
      fail("prod needs a nonempty vector or row-vector argument", e.raw);
    const bool active = a.autodiff && !in_write_array;
    const ReductionGrouping grouping = reduction_grouping(a, active);
    if (grouping == ReductionGrouping::Unknown)
      fail("prod expression grouping is not native", e.raw);
    Val result =
        with_layout(emit_value(OP_PROD_VEC, {a}, 1, {},
                               reduction_phase_idata(a, grouping, "prod")),
                    ExpressionLayout::scalar());
    // The active bit already selects scalar Matrix<var> traversal. Keep
    // the explicit scalar bit for inactive strided/gathered values so the
    // established active-vector variant remains 2.
    const bool scalar = grouping == ReductionGrouping::Scalar && !active;
    const bool phased = grouping == ReductionGrouping::Phased;
    g.ops.back().variant = static_cast<uint8_t>(
        (scalar ? 1u : 0u) | (active ? 2u : 0u) | (phased ? 4u : 0u));
    return result;
  }
  if (e.name == "sd" || e.name == "variance") {
    actuals.require_arity(1);
    Val a = actuals.at(0).value();
    if (g.slots[a.slot].len <= 0)
      fail(e.name + ": input must have a positive size", e.raw);
    return with_layout(emit_value(e.name == "sd" ? OP_SD : OP_VARIANCE, {a}, 1),
                       ExpressionLayout::scalar());
  }
  if (e.name == "rep_vector" || e.name == "rep_row_vector") {
    actuals.require_arity(2);
    Val a = actuals.at(0).value();
    if (region_current && needs_runtime_value(actuals.at(1).expr())) {
      const auto range = region_range(actuals.at(1).expr());
      if (!range || range->hi < 0)
        fail(e.name + ": runtime extent needs a finite capacity", e.raw);
      Val extent = actuals.at(1).value();
      if (!is_scalar(extent) || extent.autodiff)
        fail(e.name + ": runtime extent must be a data integer", e.raw);
      Val result = emit_value(OP_REP_VEC_DYNAMIC, {a, extent}, range->hi,
                              view_of(e.type_));
      result.runtime_dims = {extent.slot};
      return with_layout(result, owning_layout(view_of(e.type_)));
    }
    const long n = actuals.at(1).require_constant_int("rep_vector extent");
    return with_layout(emit_value(OP_REP_VEC, {a}, n, view_of(e.type_)),
                       owning_layout(view_of(e.type_)));
  }
  if (e.name == "rep_array" && e.args.size() >= 2 && e.args.size() <= 4) {
    // The element keeps its shape; rep_array prepends up to three outer
    // dimensions and tiles the element buffer once per outer cell. That
    // is a gather that walks 0..w-1 repeatedly.
    actuals.require_arity(2, 4);
    Val a = actuals.at(0).value();
    const int64_t w = g.slots[a.slot].len;
    std::vector<int64_t> dims;
    for (size_t k = 1; k < e.args.size(); ++k)
      dims.push_back(actuals.at(k).require_constant_int("rep_array extent"));
    const int64_t copies = checked_container_size(dims, e.name);
    ViewKind leaf = ViewKind::Flat;
    if (is_matrix(a.si)) {
      dims.push_back(a.si.rows);
      dims.push_back(a.si.cols);
      leaf = ViewKind::Matrix;
    } else if (is_vector(a.si)) {
      dims.push_back(w);
      leaf = ViewKind::Vector;
    } else if (is_row_vector(a.si)) {
      dims.push_back(w);
      leaf = ViewKind::RowVector;
    } else if (is_array(a.si)) {
      const ArrayShape& sh = array_shape(a.si);
      dims.insert(dims.end(), sh.dims.begin(), sh.dims.end());
      leaf = sh.leaf;
    }
    const int64_t size = checked_container_size({copies, w}, e.name);
    std::vector<int> gather;
    gather.reserve((size_t)size);
    for (int64_t k = 0; k < size; ++k)
      gather.push_back(checked_immediate(k % w, "rep_array gather offset"));
    const SlotInfo result_si = array_view(dims, leaf, a.si.param_free);
    return with_layout(emit_value(OP_GATHER, {a}, size, result_si, gather),
                       owning_layout(result_si));
  }
  if ((e.name == "zeros_vector" || e.name == "zeros_row_vector" ||
       e.name == "ones_vector" || e.name == "ones_row_vector") &&
      e.args.size() == 1) {
    // A broadcast of the constant fill, exactly as rep_vector lowers.
    actuals.require_arity(1);
    const long n = actuals.at(0).require_constant_int("vector extent");
    const double fill = e.name.rfind("ones", 0) == 0 ? 1.0 : 0.0;
    (void)checked_container_size({n}, e.name);
    const SlotInfo result_si = view_of(e.type_);
    return with_layout(emit_value(OP_REP_VEC, {constant(fill)}, n, result_si),
                       owning_layout(result_si));
  }
  if (e.name == "log_sum_exp" || e.name == "sum") {
    // The two-argument log_sum_exp overload is resolved through the
    // regular binary registry before reaching this reduction path.
    const bool int_surface =
        e.name == "sum" &&
        (e.type_ == "UInt" || e.unsized.leaf == mir::UnsizedLeaf::Int ||
         (!e.args.empty() && e.args[0].unsized.leaf == mir::UnsizedLeaf::Int));
    if (int_surface && in_write_array) {
      if (runtime_int_sum_candidate(e))
        return lower_runtime_int_sum(e, actuals);
      if (!is_int_sum_surface(e))
        fail(
            "runtime integer sum needs one one-dimensional int-array "
            "argument and a scalar int result",
            e.raw);
      if (e.args[0].kind != mir::Expr::Var || expr_effectful(e))
        fail("direct runtime integer sum stays on WaInterp", e.raw);
      // A param-free named array retains the legacy OP_SUM_VEC/fold path.
    }
    if (e.args.size() != 1)
      fail(e.name + ": reduction needs exactly one argument", e.raw);
    actuals.require_arity(1);
    Val a = actuals.at(0).value();
    if (e.name == "sum" && has_runtime_shape(a)) {
      const int extent_slot = one_runtime_extent(a, "sum");
      Val extent{extent_slot, false, view_of("UInt"),
                 ExpressionLayout::scalar()};
      extent.si.param_free = true;
      return with_layout(emit_value(OP_SUM_VEC_DYNAMIC, {a, extent}, 1),
                         ExpressionLayout::scalar());
    }
    return with_layout(
        emit_value(e.name == "sum" ? OP_SUM_VEC : OP_LOG_SUM_EXP, {a}, 1),
        ExpressionLayout::scalar());
  }
  if (e.name == "log_mix" && e.args.size() == 3) {
    Val a = actuals.at(0).value();
    Val b = actuals.at(1).value();
    Val c = actuals.at(2).value();
    return with_layout(emit_value(OP_LOG_MIX, {a, b, c}, 1),
                       ExpressionLayout::scalar());
  }
  if (e.name == "dot_product") {
    actuals.require_arity(2);
    Val a = actuals.at(0).value();
    Val b = actuals.at(1).value();
    return with_layout(emit_value(OP_DOT, {a, b}, 1),
                       ExpressionLayout::scalar());
  }

  if (e.name == "dot_self") {
    actuals.require_arity(1);
    Val a = actuals.at(0).value();
    return with_layout(emit_value(OP_DOT, {a, a}, 1),
                       ExpressionLayout::scalar());
  }

  if ((e.name == "columns_dot_product" || e.name == "rows_dot_product" ||
       e.name == "columns_dot_self" || e.name == "rows_dot_self") &&
      (e.args.size() == 1 || e.args.size() == 2)) {
    actuals.require_arity(1, 2);
    Val a = actuals.at(0).value();
    Val b = actuals.size() == 2 ? actuals.at(1).value() : a;
    if (!is_matrix(a.si) || !is_matrix(b.si) || a.si.rows != b.si.rows ||
        a.si.cols != b.si.cols)
      fail(e.name + ": arguments must be matrices of the same size", e.raw);
    SlotInfo product_si =
        matrix_view(a.si.rows, a.si.cols, a.si.param_free && b.si.param_free);
    Val product =
        with_layout(emit_value(OP_MUL, {a, b}, g.slots[a.slot].len, product_si),
                    elementwise_layout({a, b}));
    const bool by_columns = e.name.rfind("columns_", 0) == 0;
    const int64_t ones_len = by_columns ? a.si.rows : a.si.cols;
    const int ones_slot = add_slot(ones_len, false);
    out.fills.emplace_back(ones_slot, std::vector<double>(ones_len, 1.0));
    SlotInfo ones_si = view_of(by_columns ? "URowVector" : "UVector");
    ones_si.param_free = true;
    Val ones{ones_slot, false, ones_si, owning_layout(ones_si)};
    if (by_columns)
      return with_layout(
          emit_value(OP_GEMM, {ones, product}, a.si.cols, view_of("URowVector"),
                     {1, (int)a.si.rows, (int)a.si.cols}),
          owning_layout(view_of("URowVector")));
    return with_layout(
        emit_value(OP_GEMM, {product, ones}, a.si.rows, view_of("UVector"),
                   {(int)a.si.rows, (int)a.si.cols, 1}),
        owning_layout(view_of("UVector")));
  }

  if (e.name == "csr_matrix_times_vector" && e.args.size() == 6) {
    actuals.require_arity(6);
    const int64_t rows = actuals.at(0).require_constant_int("csr rows");
    const int64_t cols = actuals.at(1).require_constant_int("csr columns");
    if (rows <= 0 || cols <= 0)
      fail(e.name + ": row and column counts must be positive", e.raw);
    Val weights = actuals.at(2).value();
    Val vector = actuals.at(5).value();
    if (!is_vector(weights.si) || !is_vector(vector.si))
      fail(e.name + ": w and b must be vectors", e.raw);
    if (g.slots[vector.slot].len != cols)
      fail(e.name + ": column count does not match vector size", e.raw);
    const std::vector<int> columns =
        actuals.at(3).require_constant_ints("csr columns");
    const std::vector<int> starts =
        actuals.at(4).require_constant_ints("csr row starts");
    const int64_t nnz = g.slots[weights.slot].len;
    if ((int64_t)columns.size() != nnz)
      fail(e.name + ": w and v sizes differ", e.raw);
    if ((int64_t)starts.size() != rows + 1 || starts.front() != 1 ||
        starts.back() != nnz + 1)
      fail(e.name + ": u does not describe the requested rows", e.raw);
    for (int column : columns)
      if (column < 1 || column > cols)
        fail(e.name + ": v contains an out-of-range column", e.raw);

    Val result{-1, false, {}};
    for (int64_t row = 0; row < rows; ++row) {
      const int64_t begin = starts[(size_t)row] - 1;
      const int64_t end = starts[(size_t)row + 1] - 1;
      if (begin < 0 || end < begin || end > nnz)
        fail(e.name + ": u is not monotone or is out of range", e.raw);
      Val row_sum;
      if (begin == end) {
        row_sum = constant(0.0);
      } else {
        const int64_t len = end - begin;
        Val row_weights = with_layout(
            emit_value(OP_SLICE, {weights}, len, view_of("UVector"),
                       {checked_immediate(begin, "csr weight offset")}),
            contiguous_layout(weights, begin, "csr weights"));
        std::vector<int> gather;
        gather.reserve((size_t)len);
        for (int64_t k = begin; k < end; ++k)
          gather.push_back(columns[(size_t)k] - 1);
        Val row_vector =
            emit_value(OP_GATHER, {vector}, len, view_of("UVector"), gather);
        Val products =
            with_layout(emit_value(OP_MUL, {row_weights, row_vector}, len,
                                   view_of("UVector")),
                        elementwise_layout({row_weights, row_vector}));
        row_sum = with_layout(emit_value(OP_SUM_VEC, {products}, 1),
                              ExpressionLayout::scalar());
      }
      result = row == 0 ? row_sum
                        : with_layout(emit_value(OP_CONCAT2, {result, row_sum},
                                                 row + 1, view_of("UVector")),
                                      owning_layout(view_of("UVector")));
    }
    result.si = view_of("UVector");
    result.layout = owning_layout(result.si);
    return result;
  }

  // squared_distance(x, y) = dot_self(x - y). Two graph kernels that
  // already carry native adjoints, so no new opcode. It does not go
  // through shape_of: the language pairs a vector with a row_vector
  // here, which same_view rejects and stan-math accepts, and the only
  // thing the difference could change -- element order -- is the same
  // on both sides because a length is all either view carries.
  if (e.name == "squared_distance" && e.args.size() == 2) {
    Val a = actuals.at(0).value();
    Val b = actuals.at(1).value();
    const int64_t la = g.slots[a.slot].len, lb = g.slots[b.slot].len;
    if (la != lb) fail(e.name + ": arguments must match in size", e.raw);
    SlotInfo si;
    si.param_free = a.si.param_free && b.si.param_free;
    if (la > 1) si.kind = ViewKind::Vector;
    Val d = with_layout(emit_value(OP_SUB, {a, b}, la, si),
                        elementwise_layout({a, b}));
    return with_layout(emit_value(OP_DOT, {d, d}, 1),
                       ExpressionLayout::scalar());
  }
  return std::nullopt;
}
// Matrix shape and algebra: transposes, reshapes, factorizations,
// slices, and concatenations.
std::optional<Lowering::Val> Lowering::lower_matrix_fn(const mir::Expr& e,
                                                       CallArguments& actuals) {
  if ((e.name == "Transpose__" || e.name == "transpose") &&
      e.args.size() == 1) {
    Val a = actuals.at(0).value();
    // Vector <-> row_vector transpose is a type change, not a layout one.
    if (!is_matrix(a.si)) {
      stamp_kind(&a.si, e.type_);
      return a;
    }
    SlotInfo si = matrix_view(a.si.cols, a.si.rows, a.si.param_free);
    return with_layout(emit_value(OP_TRANSPOSE, {a}, g.slots[a.slot].len, si,
                                  {(int)a.si.rows, (int)a.si.cols}),
                       ExpressionLayout::unknown());
  }
  if (e.name == "tcrossprod" && e.args.size() == 1) {
    Val a = actuals.at(0).value();
    if (!is_matrix(a.si)) fail("tcrossprod: needs a matrix", e.raw);
    SlotInfo transpose_si = matrix_view(a.si.cols, a.si.rows, a.si.param_free);
    Val transpose =
        with_layout(emit_value(OP_TRANSPOSE, {a}, g.slots[a.slot].len,
                               transpose_si, {(int)a.si.rows, (int)a.si.cols}),
                    a.layout);
    SlotInfo si = matrix_view(a.si.rows, a.si.rows, a.si.param_free);
    return with_layout(
        emit_value(OP_GEMM, {a, transpose}, a.si.rows * a.si.rows, si,
                   {(int)a.si.rows, (int)a.si.cols, (int)a.si.rows}),
        owning_layout(si));
  }
  if (e.name == "crossprod" && e.args.size() == 1) {
    Val a = actuals.at(0).value();
    if (!is_matrix(a.si)) fail("crossprod: needs a matrix", e.raw);
    SlotInfo si = matrix_view(a.si.cols, a.si.cols, a.si.param_free);
    Val v = emit_value(OP_CROSSPROD, {a}, a.si.cols * a.si.cols, si,
                       {checked_immediate(a.si.rows, "crossprod rows"),
                        checked_immediate(a.si.cols, "crossprod cols")});
    g.ops.back().variant = v.autodiff ? 1u : 0u;
    return with_layout(v, owning_layout(si));
  }
  if ((e.name == "diag_pre_multiply" || e.name == "diag_post_multiply") &&
      e.args.size() == 2) {
    // diag_pre_multiply(v, M) = diag_matrix(v) * M (and the mirror);
    // the explicit zeros contribute exactly nothing to each sum.
    const bool pre = e.name.find("_pre_") != std::string::npos;
    Val v = actuals.at(pre ? 0 : 1).value();
    Val m = actuals.at(pre ? 1 : 0).value();
    const int64_t n = g.slots[v.slot].len;
    SlotInfo dsi = matrix_view(n, n, v.si.param_free);
    Val d = with_layout(emit_value(OP_DIAG_MATRIX, {v}, n * n, dsi),
                        owning_layout(dsi));
    Val a = pre ? d : m, b = pre ? m : d;
    SlotInfo si = matrix_view(a.si.rows, b.si.cols);
    return with_layout(
        emit_value(OP_GEMM, {a, b}, si.rows * si.cols, si,
                   {(int)a.si.rows, (int)a.si.cols, (int)b.si.cols}),
        owning_layout(si));
  }
  if (e.name == "multiply_lower_tri_self_transpose" && e.args.size() == 1) {
    // Not L * L': stan-math drops L's upper triangle first, and only a
    // cholesky_factor_* value already has zeros there. A TRANSPOSE/GEMM
    // pair would read the dropped entries and disagree on every result
    // touching one.
    Val L = actuals.at(0).value();
    if (!is_matrix(L.si)) fail("multiply_lower_tri: needs a matrix", e.raw);
    SlotInfo si = matrix_view(L.si.rows, L.si.rows, L.si.param_free);
    Val v = with_layout(
        emit_value(OP_MULT_LOWER_TRI_SELF_TRANSPOSE, {L}, L.si.rows * L.si.rows,
                   si,
                   {checked_immediate(L.si.rows, "multiply_lower_tri rows"),
                    checked_immediate(L.si.cols, "multiply_lower_tri cols")}),
        owning_layout(si));
    g.ops.back().variant = v.autodiff ? 1u : 0u;
    return v;
  }
  if (e.name == "to_matrix" && (e.args.size() == 1 || e.args.size() == 3)) {
    // Col-major storage makes reshaping a relabelling. One argument on an
    // array[N] vector[S] value yields the N x S matrix stan-math builds
    // from it, which is the transpose of our array-major flat order.
    Val a = actuals.at(0).value();
    SlotInfo si;
    si.param_free = a.si.param_free;
    if (e.args.size() == 3) {
      const int64_t rows = actuals.at(1).require_constant_int("to_matrix rows");
      const int64_t cols = actuals.at(2).require_constant_int("to_matrix cols");
      if (checked_product({rows, cols}, "to_matrix") != g.slots[a.slot].len)
        fail("to_matrix: requested shape does not match source length", e.raw);
      si = matrix_view(rows, cols, a.si.param_free);
      return Val{a.slot, a.autodiff, si, owning_layout(si)};
    }
    if (is_matrix(a.si)) return Val{a.slot, a.autodiff, a.si, a.layout};
    if (is_vector(a.si)) {
      const SlotInfo result_si =
          matrix_view(g.slots[a.slot].len, 1, a.si.param_free);
      return Val{a.slot, a.autodiff, result_si, owning_layout(result_si)};
    }
    if (is_row_vector(a.si)) {
      const SlotInfo result_si =
          matrix_view(1, g.slots[a.slot].len, a.si.param_free);
      return Val{a.slot, a.autodiff, result_si, owning_layout(result_si)};
    }
    std::vector<int64_t> dims;
    if (is_array(a.si)) dims = array_shape(a.si).dims;
    if (dims.size() != 2) fail("to_matrix: unknown source shape", e.raw);
    if (dims[0] == 0) dims[1] = 0;
    // array-major (row-major) source -> col-major matrix of the same
    // logical shape: transpose the storage.
    si = matrix_view(dims[0], dims[1], a.si.param_free);
    return with_layout(emit_value(OP_TRANSPOSE, {a}, g.slots[a.slot].len, si,
                                  {(int)dims[1], (int)dims[0]}),
                       owning_layout(si));
  }
  if ((e.name == "to_vector" || e.name == "to_row_vector") &&
      e.args.size() == 1) {
    // Col-major flattening is the identity on our storage.
    Val a = actuals.at(0).value();
    SlotInfo si = view_of(e.type_);
    si.param_free = a.si.param_free;
    return Val{a.slot, a.autodiff, si, owning_layout(si)};
  }
  if (e.name == "to_array_1d" && e.args.size() == 1) {
    Val a = actuals.at(0).value();
    SlotInfo si =
        array_view({g.slots[a.slot].len}, ViewKind::Flat, a.si.param_free);
    return Val{a.slot, a.autodiff, si, owning_layout(si)};
  }
  if (e.name == "rep_matrix") {
    SlotInfo si;
    if (e.args.size() == 3) {
      Val x = actuals.at(0).value();  // scalar fill
      const long R = actuals.at(1).require_constant_int("rep_matrix rows");
      const long C = actuals.at(2).require_constant_int("rep_matrix cols");
      si = matrix_view(R, C);
      return with_layout(
          emit_value(OP_REP_MAT, {x}, R * C, si, {(int)R, (int)C, 0}),
          owning_layout(si));
    }
    if (e.args.size() == 2) {
      Val v = actuals.at(0).value();
      const long n = actuals.at(1).require_constant_int("rep_matrix extent");
      const bool rowvec = actuals.at(0).expr().type_ == "URowVector";
      const long R = rowvec ? n : g.slots[v.slot].len;
      const long C = rowvec ? g.slots[v.slot].len : n;
      si = matrix_view(R, C);
      return with_layout(emit_value(OP_REP_MAT, {v}, R * C, si,
                                    {(int)R, (int)C, rowvec ? 2 : 1}),
                         owning_layout(si));
    }
    fail("rep_matrix arity", e.raw);
  }
  if (const std::optional<GpCov> gp = gp_cov_family(e.name);
      gp && e.args.size() == 3) {
    Val x = actuals.at(0).value();
    Val alpha = actuals.at(1).value();
    Val rho = actuals.at(2).value();
    // x may be data or a parameter: gp_cov_bwd rebuilds the points from
    // the promoted input, so a parameter x gets its adjoints too.
    // x is array[N] real (D == 1) or array[N] vector[D], stored
    // array-major, so D falls out of the declared dims.
    int64_t D = 1;
    if (is_array(x.si) && array_shape(x.si).dims.size() == 2)
      D = array_shape(x.si).dims[1];
    const int64_t N = g.slots[x.slot].len / D;
    SlotInfo si = matrix_view(N, N);
    Val v = emit_value(OP_GP_COV, {x, alpha, rho}, N * N, si, {(int)N, (int)D});
    g.ops.back().variant = static_cast<uint8_t>(*gp);
    return with_layout(v, owning_layout(si));
  }
  if (e.name == "diag_matrix" && e.args.size() == 1) {
    Val v = actuals.at(0).value();
    const int64_t n = g.slots[v.slot].len;
    SlotInfo si = matrix_view(n, n);
    return with_layout(emit_value(OP_DIAG_MATRIX, {v}, n * n, si),
                       owning_layout(si));
  }
  if (e.name == "cholesky_decompose" && e.args.size() == 1) {
    Val a = actuals.at(0).value();
    if (!is_matrix(a.si)) fail("cholesky_decompose needs a matrix", e.raw);
    if (a.si.rows != a.si.cols)
      fail("cholesky_decompose needs a square matrix", e.raw);
    SlotInfo si = a.si;
    si.param_free = a.si.param_free;
    return with_layout(
        emit_value(OP_CHOLESKY, {a}, g.slots[a.slot].len, si, {(int)a.si.rows}),
        owning_layout(si));
  }
  if (e.name == "matrix_exp" && e.args.size() == 1) {
    Val a = actuals.at(0).value();
    if (!is_matrix(a.si)) fail("matrix_exp: needs a matrix", e.raw);
    if (a.si.rows != a.si.cols)
      fail("matrix_exp: needs a square matrix", e.raw);
    if (has_runtime_shape(a)) {
      if (a.runtime_dims.size() != 2 || a.runtime_dims[0] < 0 ||
          a.runtime_dims[0] != a.runtime_dims[1])
        fail("matrix_exp: needs one runtime square extent", e.raw);
      Val extent{a.runtime_dims[0], false, view_of("UInt"),
                 ExpressionLayout::scalar()};
      extent.si.param_free = true;
      Val result = emit_value(OP_MATRIX_EXP_DYNAMIC, {a, extent},
                              g.slots[a.slot].len, a.si);
      result.runtime_dims = a.runtime_dims;
      return with_layout(result, owning_layout(a.si));
    }
    return with_layout(
        emit_value(OP_MATRIX_EXP, {a}, g.slots[a.slot].len, a.si,
                   {checked_immediate(a.si.rows, "matrix_exp extent")}),
        owning_layout(a.si));
  }
  if ((e.name == "inverse" || e.name == "inverse_spd") && e.args.size() == 1) {
    Val a = actuals.at(0).value();
    if (!is_matrix(a.si)) fail(e.name + ": needs a matrix", e.raw);
    if (a.si.rows != a.si.cols) fail(e.name + ": needs a square matrix", e.raw);
    Val v = emit_value(e.name == "inverse" ? OP_INVERSE : OP_INVERSE_SPD, {a},
                       g.slots[a.slot].len, a.si,
                       {checked_immediate(a.si.rows, e.name + " extent")});
    if (e.name == "inverse_spd") g.ops.back().variant = v.autodiff ? 1u : 0u;
    return with_layout(v, owning_layout(a.si));
  }
  if (e.name == "log_determinant" && e.args.size() == 1) {
    Val a = actuals.at(0).value();
    if (!is_matrix(a.si)) fail("log_determinant: needs a matrix", e.raw);
    if (a.si.rows != a.si.cols)
      fail("log_determinant: needs a square matrix", e.raw);
    return with_layout(
        emit_value(OP_LOG_DETERMINANT, {a}, 1, {},
                   {checked_immediate(a.si.rows, "log_determinant extent")}),
        ExpressionLayout::scalar());
  }

  if ((e.name == "eigenvalues_sym" || e.name == "eigenvectors_sym") &&
      e.args.size() == 1) {
    Val a = actuals.at(0).value();
    if (!is_matrix(a.si)) fail(e.name + ": needs a matrix", e.raw);
    if (a.si.rows != a.si.cols) fail(e.name + ": needs a square matrix", e.raw);
    const int64_t n = a.si.rows;
    if (e.name == "eigenvalues_sym")
      return with_layout(
          emit_value(OP_EIGENVALUES_SYM, {a}, n, view_of(e.type_), {(int)n}),
          owning_layout(view_of(e.type_)));
    SlotInfo si = matrix_view(n, n);
    return with_layout(
        emit_value(OP_EIGENVECTORS_SYM, {a}, n * n, si, {(int)n}),
        owning_layout(si));
  }
  if (e.name == "quad_form_diag" && e.args.size() == 2) {
    // quad_form_diag(M, v) = diag(v) * M * diag(v).
    Val m = actuals.at(0).value();
    Val v = actuals.at(1).value();
    if (!is_matrix(m.si)) fail("quad_form_diag: needs a matrix", e.raw);
    const int64_t n = g.slots[v.slot].len;
    SlotInfo dsi = matrix_view(n, n, v.si.param_free);
    Val d = with_layout(emit_value(OP_DIAG_MATRIX, {v}, n * n, dsi),
                        owning_layout(dsi));
    SlotInfo si = matrix_view(n, n);
    Val left = with_layout(
        emit_value(OP_GEMM, {d, m}, n * n, si, {(int)n, (int)n, (int)n}),
        owning_layout(si));
    return with_layout(
        emit_value(OP_GEMM, {left, d}, n * n, si, {(int)n, (int)n, (int)n}),
        owning_layout(si));
  }

  if (e.name == "quad_form_sym" && e.args.size() == 2) {
    // 0.5 * (C + C') with C = B' A B, and the plain scalar B' A B when B
    // is a vector. This stays one op rather than a transpose and two
    // GEMMs because stan-math's own association is part of the answer:
    // the kernel makes the same calls CmdStan does, including the
    // symmetry check on A, which throws when A is only nearly symmetric.
    Val a = actuals.at(0).value();
    Val b = actuals.at(1).value();
    if (!is_matrix(a.si)) fail("quad_form_sym: needs a matrix", e.raw);
    if (a.si.rows != a.si.cols)
      fail("quad_form_sym: needs a square matrix", e.raw);
    const bool b_matrix = is_matrix(b.si);
    if (!b_matrix && !is_vector(b.si))
      fail("quad_form_sym: second argument is not a matrix or vector", e.raw);
    const int64_t n = a.si.rows;
    const int64_t rb = b_matrix ? b.si.rows : g.slots[b.slot].len;
    const int64_t m = b_matrix ? b.si.cols : 1;
    if (rb != n)
      fail("quad_form_sym: inner dimension mismatch (" + std::to_string(n) +
               "x" + std::to_string(n) + " against " + std::to_string(rb) + ")",
           e.raw);
    const SlotInfo si = b_matrix ? matrix_view(m, m) : SlotInfo{};
    Val v = emit_value(OP_QUAD_FORM_SYM, {a, b}, m * m, si,
                       {checked_immediate(n, "quad_form_sym extent"),
                        checked_immediate(m, "quad_form_sym extent")});
    // Bit 0 is the operand shape. Bit 1 says CmdStan would have typed
    // this expression `var`, which for a vector B picks stan-math's other
    // association of the same product -- the same distinction the matrix
    // solves make, and for the same reason.
    g.ops.back().variant =
        (uint8_t)((b_matrix ? 0u : 1u) | (v.autodiff ? 2u : 0u));
    return with_layout(
        v, b_matrix ? owning_layout(si) : ExpressionLayout::scalar());
  }

  if (e.name == "quad_form" && e.args.size() == 2) {
    Val a = actuals.at(0).value();
    Val b = actuals.at(1).value();
    if (!is_matrix(a.si)) fail("quad_form: needs a matrix", e.raw);
    if (a.si.rows != a.si.cols) fail("quad_form: needs a square matrix", e.raw);
    const bool b_matrix = is_matrix(b.si);
    if (!b_matrix && !is_vector(b.si))
      fail("quad_form: second argument is not a matrix or vector", e.raw);
    const int64_t n = a.si.rows;
    const int64_t rb = b_matrix ? b.si.rows : g.slots[b.slot].len;
    const int64_t m = b_matrix ? b.si.cols : 1;
    if (rb != n)
      fail("quad_form: inner dimension mismatch (" + std::to_string(n) + "x" +
               std::to_string(n) + " against " + std::to_string(rb) + ")",
           e.raw);
    const SlotInfo si = b_matrix ? matrix_view(m, m) : SlotInfo{};
    Val v = emit_value(OP_QUAD_FORM, {a, b}, m * m, si,
                       {checked_immediate(n, "quad_form extent"),
                        checked_immediate(m, "quad_form extent")});
    g.ops.back().variant =
        (uint8_t)((b_matrix ? 0u : 1u) | (v.autodiff ? 2u : 0u));
    return with_layout(
        v, b_matrix ? owning_layout(si) : ExpressionLayout::scalar());
  }

  if (e.name == "add_diag" && e.args.size() == 2) {
    Val a = actuals.at(0).value();
    Val d = actuals.at(1).value();
    if (!is_matrix(a.si)) fail("add_diag: needs a matrix", e.raw);
    const bool scalar = is_scalar(d);
    const int64_t n = std::min(a.si.rows, a.si.cols);
    if (!scalar && !is_vector(d.si) && !is_row_vector(d.si))
      fail("add_diag: diagonal must be a scalar or vector", e.raw);
    if (!scalar && g.slots[d.slot].len != n)
      fail("add_diag: diagonal length mismatch", e.raw);
    Val v = emit_value(OP_ADD_DIAG, {a, d}, g.slots[a.slot].len, a.si,
                       {checked_immediate(a.si.rows, "add_diag rows"),
                        checked_immediate(a.si.cols, "add_diag cols")});
    g.ops.back().variant = scalar ? 1u : 0u;
    return with_layout(v, owning_layout(a.si));
  }

  if ((e.name == "append_row" || e.name == "append_col") &&
      e.args.size() == 2) {
    Val a = actuals.at(0).value();
    Val b = actuals.at(1).value();
    const int64_t la = g.slots[a.slot].len, lb = g.slots[b.slot].len;
    const LogicalDims da = logical_dims(a.si, la, e.name);
    const LogicalDims db = logical_dims(b.si, lb, e.name);
    if (e.name == "append_col") {
      if (da.rows != db.rows) fail("append_col row mismatch", e.raw);
      const LogicalDims out_dims{da.rows, da.cols + db.cols};
      const SlotInfo si = view_for_dims(e.type_, out_dims);
      // Every supported value is column-major under this logical view;
      // adding columns is therefore always a contiguous concatenation.
      return with_layout(emit_value(OP_CONCAT2, {a, b}, la + lb, si),
                         owning_layout(si));
    }
    if (da.cols != db.cols) fail("append_row column mismatch", e.raw);
    const LogicalDims out_dims{da.rows + db.rows, da.cols};
    const SlotInfo si = view_for_dims(e.type_, out_dims);
    if (out_dims.cols == 1)
      return with_layout(emit_value(OP_CONCAT2, {a, b}, la + lb, si),
                         owning_layout(si));

    // Adding rows interleaves the two column-major operands one column at
    // a time. The same gather handles row-vectors and mixed matrix+row.
    Val cat = emit_value(OP_CONCAT2, {a, b}, la + lb, {});
    std::vector<int> idx;
    idx.reserve((size_t)(la + lb));
    for (int64_t j = 0; j < out_dims.cols; ++j) {
      for (int64_t i = 0; i < da.rows; ++i)
        idx.push_back((int)(j * da.rows + i));
      for (int64_t i = 0; i < db.rows; ++i)
        idx.push_back((int)(la + j * db.rows + i));
    }
    return with_layout(emit_value(OP_GATHER, {cat}, la + lb, si, idx),
                       owning_layout(si));
  }
  if (e.name == "segment" && e.args.size() == 3) {
    Val a = actuals.at(0).value();
    const long from = actuals.at(1).require_constant_int("segment start");
    const long cnt = actuals.at(2).require_constant_int("segment count");
    const int64_t offset = from - 1;
    const int immediate = checked_immediate(offset, "segment offset");
    return with_layout(
        emit_value(OP_SLICE, {a}, cnt, view_of(e.type_), {immediate}),
        contiguous_layout(a, offset, "segment"));
  }
  if (e.name == "sub_col" && e.args.size() == 4) {
    // sub_col(M, i, j, n) = M[i .. i+n-1, j]: contiguous in col-major.
    Val a = actuals.at(0).value();
    if (!is_matrix(a.si)) fail("sub_col on a slot without matrix shape");
    const long i = actuals.at(1).require_constant_int("sub_col row");
    const long j = actuals.at(2).require_constant_int("sub_col column");
    const long n = actuals.at(3).require_constant_int("sub_col count");
    const int64_t offset = (j - 1) * a.si.rows + i - 1;
    const int immediate = checked_immediate(offset, "sub_col offset");
    return with_layout(
        emit_value(OP_SLICE, {a}, n, view_of(e.type_), {immediate}),
        contiguous_layout(a, offset, "sub_col"));
  }
  if (e.name == "block" && e.args.size() == 5) {
    // block(M, i, j, nr, nc) = M[i .. i+nr-1, j .. j+nc-1]. Each result
    // column is contiguous in col-major storage, but consecutive result
    // columns sit M.rows apart, so a 2-D window needs a gather rather
    // than the single slice sub_col gets.
    Val a = actuals.at(0).value();
    if (!is_matrix(a.si)) fail("block on a slot without matrix shape");
    const long i = actuals.at(1).require_constant_int("block row");
    const long j = actuals.at(2).require_constant_int("block column");
    const long nr = actuals.at(3).require_constant_int("block rows");
    const long nc = actuals.at(4).require_constant_int("block columns");
    check_block_shape(a.si.rows, a.si.cols, i, j, nr, nc);
    std::vector<int> gather;
    gather.reserve((size_t)(nr * nc));
    for (long c = 0; c < nc; ++c)
      for (long k = 0; k < nr; ++k)
        gather.push_back(checked_immediate(
            (j - 1 + c) * a.si.rows + (i - 1 + k), "block gather offset"));
    return with_layout(
        emit_value(OP_GATHER, {a}, nr * nc, matrix_view(nr, nc), gather),
        ExpressionLayout::scalar());
  }
  if (e.name == "col" && e.args.size() == 2) {
    Val a = actuals.at(0).value();
    if (!is_matrix(a.si)) fail("col on a slot without matrix shape");
    const long j = actuals.at(1).require_constant_int("col index");
    const int64_t offset = (j - 1) * a.si.rows;
    const int immediate = checked_immediate(offset, "col offset");
    return with_layout(
        emit_value(OP_SLICE, {a}, a.si.rows, view_of(e.type_), {immediate}),
        contiguous_layout(a, offset, "col"));
  }
  if (e.name == "diagonal" && e.args.size() == 1) {
    Val a = actuals.at(0).value();
    if (!is_matrix(a.si)) fail("diagonal on a slot without matrix shape");
    // Eigen's diagonal steps one row and one column at a time, which in
    // column-major storage is rows + 1 apart, and stops at the shorter
    // side.
    const int64_t n = std::min(a.si.rows, a.si.cols);
    return with_layout(
        emit_value(OP_SLICE_STRIDED, {a}, n, view_of(e.type_),
                   {0, checked_immediate(a.si.rows + 1, "diagonal stride")}),
        ExpressionLayout::scalar());
  }
  if (e.name == "row" && e.args.size() == 2) {
    Val a = actuals.at(0).value();
    if (!is_matrix(a.si)) fail("row on a slot without matrix shape");
    const long i = actuals.at(1).require_constant_int("row index");
    const int64_t offset = i - 1;
    return with_layout(
        emit_value(OP_SLICE_STRIDED, {a}, a.si.cols, view_of(e.type_),
                   {checked_immediate(offset, "row offset"),
                    checked_immediate(a.si.rows, "row stride")}),
        ExpressionLayout::scalar());
  }
  if ((e.name == "head" || e.name == "tail") && e.args.size() == 2) {
    Val a = actuals.at(0).value();
    const long n = actuals.at(1).require_constant_int("head/tail count");
    const long off = e.name == "head" ? 0 : g.slots[a.slot].len - n;
    const int immediate = checked_immediate(off, e.name + " offset");
    return with_layout(
        emit_value(OP_SLICE, {a}, n, view_of(e.type_), {immediate}),
        contiguous_layout(a, off, e.name));
  }
  if (e.name == "reverse" && e.args.size() == 1) {
    Val a = actuals.at(0).value();
    const int64_t len = g.slots[a.slot].len;
    std::vector<int> gather;
    gather.reserve((size_t)len);
    if (is_array(a.si)) {
      // Graph arrays keep each outer element contiguous. Reverse those
      // complete chunks so an array of vectors/matrices retains the order
      // inside every element.
      const ArrayShape& shape = array_shape(a.si);
      if (shape.dims.empty()) fail("reverse: array has no dimensions", e.raw);
      const int64_t outer = shape.dims.front();
      const std::vector<int64_t> suffix(shape.dims.begin() + 1,
                                        shape.dims.end());
      const int64_t width = checked_product(suffix, "reverse array element");
      if (checked_product(shape.dims, "reverse array") != len)
        fail("reverse: array shape does not match storage", e.raw);
      for (int64_t i = outer; i-- > 0;)
        for (int64_t k = 0; k < width; ++k)
          gather.push_back(
              checked_immediate(i * width + k, "reverse gather offset"));
    } else {
      if (!is_vector(a.si) && !is_row_vector(a.si))
        fail("reverse: argument is not a vector, row-vector, or array", e.raw);
      for (int64_t i = len; i-- > 0;)
        gather.push_back(checked_immediate(i, "reverse gather offset"));
    }
    return with_layout(emit_value(OP_GATHER, {a}, len, a.si, gather),
                       ExpressionLayout::scalar());
  }
  return std::nullopt;
}
}  // namespace lower_detail
}  // namespace stanli
