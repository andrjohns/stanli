#include <stanrt/compile.hpp>
#include <stanrt/mir.hpp>
#include <stanrt/optable.hpp>
#include <stanrt/sexp.hpp>

#include <map>
#include <string>
#include <vector>

namespace stanrt {
namespace {

struct SlotInfo {
  int64_t len = 1;
  int64_t rows = 0, cols = 0;  // set for matrices
  bool data_like = false;      // no adjoint (data or constant)
};

struct Lowering {
  const DataMap& data;
  Graph g;
  CompiledModel out;
  std::map<std::string, int> scope;            // var -> slot
  std::map<std::string, long> int_env;         // data int scalars
  std::map<std::string, std::string> int_arrays;  // int-array vars (by name)
  std::map<double, int> const_cache;
  std::vector<SlotInfo> info;                  // parallel to g.slots
  std::vector<int> target_terms;
  std::vector<int> jac_slots;

  explicit Lowering(const DataMap& d) : data(d) {}

  int add_slot(int64_t len, bool is_param, SlotInfo si = {}) {
    si.len = len;
    const int s = g.add_slot(len, is_param);
    info.push_back(si);
    return s;
  }

  [[noreturn]] void fail(const std::string& msg, const std::string& raw = "") {
    throw CompileError("stanrt compile: " + msg +
                       (raw.empty() ? "" : "\n  in: " + raw));
  }

  int const_slot(double v) {
    auto it = const_cache.find(v);
    if (it != const_cache.end()) return it->second;
    SlotInfo si;
    si.data_like = true;
    const int s = add_slot(1, false, si);
    out.fills.emplace_back(s, std::vector<double>{v});
    const_cache[v] = s;
    return s;
  }

  long eval_int(const mir::Expr& e) {
    switch (e.kind) {
      case mir::Expr::LitInt:
        return e.lit_i;
      case mir::Expr::Var: {
        auto it = int_env.find(e.name);
        if (it == int_env.end()) fail("size expression needs unknown int " + e.name);
        return it->second;
      }
      case mir::Expr::FunApp:
        if (e.name == "Plus__") return eval_int(e.args[0]) + eval_int(e.args[1]);
        if (e.name == "Minus__") return eval_int(e.args[0]) - eval_int(e.args[1]);
        if (e.name == "Times__") return eval_int(e.args[0]) * eval_int(e.args[1]);
        fail("unsupported int size function " + e.name, e.raw);
      default:
        fail("unsupported size expression", e.raw);
    }
  }

  int64_t sized_len(const mir::SizedType& t, int64_t* rows = nullptr,
                    int64_t* cols = nullptr) {
    if (t.base == "SInt" || t.base == "SReal") return 1;
    if (t.base == "SVector" || t.base == "SRowVector")
      return eval_int(t.dims[0]);
    if (t.base == "SMatrix") {
      const int64_t r = eval_int(t.dims[0]), c = eval_int(t.dims[1]);
      if (rows) *rows = r;
      if (cols) *cols = c;
      return r * c;
    }
    if (t.base == "SArray") {
      int64_t n = 1;
      for (const auto& d : t.dims) n *= eval_int(const_cast<mir::Expr&>(d));
      return n;
    }
    fail("unsupported sized type " + t.base, t.raw);
  }

  // ---- data block -----------------------------------------------------------
  void bind_data(const mir::Program& p) {
    for (const auto& [name, type] : p.input_vars) {
      const DataMap::Entry& e = data.at(name);
      if (type.base == "SInt") {
        if (!e.is_int) fail("data " + name + " must be int");
        int_env[name] = e.i.at(0);
        continue;  // int scalars live in the int env, not the arena
      }
      if (type.base == "SArray" && !e.r.size() && e.is_int) {
        // Int arrays: kept by name for density outcomes (idata), and also
        // exposed as a double slot if arithmetic needs them later.
        int_arrays[name] = name;
        continue;
      }
      int64_t rows = 0, cols = 0;
      const int64_t len = sized_len(type, &rows, &cols);
      if ((int64_t)e.r.size() != len)
        fail("data " + name + ": expected " + std::to_string(len) +
             " values, got " + std::to_string(e.r.size()));
      SlotInfo si;
      si.data_like = true;
      si.rows = rows;
      si.cols = cols;
      const int s = add_slot(len, false, si);
      out.fills.emplace_back(s, e.r);
      scope[name] = s;
    }
  }

  // ---- expressions ----------------------------------------------------------
  struct Val {
    int slot;
    SlotInfo si;
  };

  Val lower_expr(const mir::Expr& e) {
    switch (e.kind) {
      case mir::Expr::Var: {
        auto it = scope.find(e.name);
        if (it == scope.end()) {
          auto ii = int_env.find(e.name);
          if (ii != int_env.end())
            return {const_slot(static_cast<double>(ii->second)), {}};
          fail("unknown variable " + e.name);
        }
        return {it->second, info[it->second]};
      }
      case mir::Expr::LitInt:
        return {const_slot(static_cast<double>(e.lit_i)), {}};
      case mir::Expr::LitReal:
        return {const_slot(e.lit), {}};
      case mir::Expr::FunApp:
        return lower_funapp(e);
      default:
        fail("unsupported expression", e.raw.empty() ? e.name : e.raw);
    }
  }

  Val emit(uint16_t opcode, std::vector<int> ins, int64_t out_len,
           SlotInfo out_si = {}, std::vector<int> idata = {}, int out2 = -1) {
    const int o = add_slot(out_len, false, out_si);
    Op op;
    op.opcode = opcode;
    op.out = o;
    op.out2 = out2;
    op.n_in = 0;
    for (int s : ins) op.in[op.n_in++] = s;
    if (!idata.empty()) {
      g.idata_pool.push_back(std::move(idata));
      op.idata = g.idata_pool.back().data();
      op.n_idata = (int64_t)g.idata_pool.back().size();
    }
    g.ops.push_back(op);
    return {o, info[o]};
  }

  Val lower_funapp(const mir::Expr& e) {
    if (e.fn_lib != mir::Expr::Lib::StanLib)
      fail("unsupported function kind for " + e.name, e.raw);

    // Densities. n_int leading args come from int data (idata); the rest are
    // real slots. Layouts: one int group = raw values; two groups =
    // [len, vals..., len, vals...]; glm = [y..., rows, cols].
    struct Dens { uint16_t op; int nargs; int n_int; bool glm = false; };
    static const std::map<std::string, Dens> kDens = {
        {"normal_lpdf", {OP_NORMAL_LPDF, 3, 0}},
        {"cauchy_lpdf", {OP_CAUCHY_LPDF, 3, 0}},
        {"student_t_lpdf", {OP_STUDENT_T_LPDF, 4, 0}},
        {"gamma_lpdf", {OP_GAMMA_LPDF, 3, 0}},
        {"beta_lpdf", {OP_BETA_LPDF, 3, 0}},
        {"lognormal_lpdf", {OP_LOGNORMAL_LPDF, 3, 0}},
        {"uniform_lpdf", {OP_UNIFORM_LPDF, 3, 0}},
        {"double_exponential_lpdf", {OP_DOUBLE_EXP_LPDF, 3, 0}},
        {"exponential_lpdf", {OP_EXPONENTIAL_LPDF, 2, 0}},
        {"inv_gamma_lpdf", {OP_INV_GAMMA_LPDF, 3, 0}},
        {"std_normal_lpdf", {OP_STD_NORMAL_LPDF, 1, 0}},
        {"poisson_log_lpmf", {OP_POISSON_LOG_LPMF, 2, 1}},
        {"bernoulli_logit_lpmf", {OP_BERNOULLI_LOGIT_LPMF, 2, 1}},
        {"bernoulli_lpmf", {OP_BERNOULLI_LPMF, 2, 1}},
        {"poisson_lpmf", {OP_POISSON_LPMF, 2, 1}},
        {"neg_binomial_2_lpmf", {OP_NEG_BINOMIAL_2_LPMF, 3, 1}},
        {"binomial_lpmf", {OP_BINOMIAL_LPMF, 3, 2}},
        {"binomial_logit_lpmf", {OP_BINOMIAL_LOGIT_LPMF, 3, 2}},
        {"bernoulli_logit_glm_lpmf",
         {OP_BERNOULLI_LOGIT_GLM_LPMF, 4, 1, true}},
    };
    auto dit = kDens.find(e.name);
    if (dit != kDens.end()) {
      const Dens& d = dit->second;
      if ((int)e.args.size() != d.nargs)
        fail(e.name + ": expected " + std::to_string(d.nargs) + " args");
      auto int_arg = [&](const mir::Expr& oc) -> std::vector<int> {
        if (oc.kind == mir::Expr::Var && data.has(oc.name) &&
            data.at(oc.name).is_int)
          return data.at(oc.name).i;
        if (oc.kind == mir::Expr::LitInt)
          return {static_cast<int>(oc.lit_i)};
        if (oc.kind == mir::Expr::Var && int_env.count(oc.name))
          return {static_cast<int>(int_env[oc.name])};
        fail(e.name + ": int argument must be int data in M2", oc.raw);
      };
      std::vector<int> idata;
      if (d.n_int == 1) {
        idata = int_arg(e.args[0]);
      } else if (d.n_int == 2) {
        auto g1 = int_arg(e.args[0]), g2 = int_arg(e.args[1]);
        idata.push_back((int)g1.size());
        idata.insert(idata.end(), g1.begin(), g1.end());
        idata.push_back((int)g2.size());
        idata.insert(idata.end(), g2.begin(), g2.end());
      }
      std::vector<int> ins;
      for (size_t i = d.n_int; i < e.args.size(); ++i)
        ins.push_back(lower_expr(e.args[i]).slot);
      if (d.glm) {
        // X must be a data matrix; append its dims to idata.
        const SlotInfo& xsi = info[ins[0]];
        if (xsi.rows == 0 || !xsi.data_like)
          fail(e.name + ": X must be a data matrix in M2");
        idata.push_back((int)xsi.rows);
        idata.push_back((int)xsi.cols);
      }
      return emit(d.op, ins, 1, {}, idata);
    }

    // Elementwise binaries.
    static const std::map<std::string, uint16_t> kBin = {
        {"Plus__", OP_ADD},      {"Minus__", OP_SUB},
        {"Divide__", OP_DIV},    {"EltTimes__", OP_MUL},
        {"EltDivide__", OP_DIV}, {"Pow__", OP_POW},
    };
    if (e.name == "Times__") {
      Val a = lower_expr(e.args[0]);
      Val b = lower_expr(e.args[1]);
      if (a.si.rows > 0) {  // matrix * vector
        if (!info[a.slot].data_like)
          fail("parameter matrix in Times__ unsupported in M2");
        SlotInfo si;
        return emit(OP_MATVEC, {a.slot, b.slot}, a.si.rows, si,
                    {(int)a.si.rows, (int)a.si.cols});
      }
      const int64_t len = std::max(info[a.slot].len, info[b.slot].len);
      return emit(OP_MUL, {a.slot, b.slot}, len);
    }
    auto bit = kBin.find(e.name);
    if (bit != kBin.end()) {
      Val a = lower_expr(e.args[0]);
      Val b = lower_expr(e.args[1]);
      const int64_t la = info[a.slot].len, lb = info[b.slot].len;
      if (la != lb && la != 1 && lb != 1)
        fail(e.name + ": incompatible lengths");
      return emit(bit->second, {a.slot, b.slot}, std::max(la, lb));
    }

    // Elementwise unaries + reductions.
    static const std::map<std::string, uint16_t> kUn = {
        {"PMinus__", OP_NEG}, {"exp", OP_EXPV},      {"log", OP_LOGV},
        {"inv_logit", OP_INV_LOGIT}, {"sqrt", OP_SQRT},
        {"square", OP_SQUARE}, {"log1m", OP_LOG1M},  {"softmax", OP_SOFTMAX},
    };
    auto uit = kUn.find(e.name);
    if (uit != kUn.end()) {
      Val a = lower_expr(e.args[0]);
      return emit(uit->second, {a.slot}, info[a.slot].len);
    }
    if (e.name == "PPlus__") return lower_expr(e.args[0]);
    if (e.name == "logit") {
      Val a = lower_expr(e.args[0]);
      return emit(OP_LOGIT, {a.slot}, info[a.slot].len);
    }
    if (e.name == "mean") {
      Val a = lower_expr(e.args[0]);
      return emit(OP_MEAN, {a.slot}, 1);
    }
    if (e.name == "rep_vector") {
      Val a = lower_expr(e.args[0]);
      const long n = eval_int(e.args[1]);
      return emit(OP_REP_VEC, {a.slot}, n);
    }
    if (e.name == "log_sum_exp" || e.name == "sum") {
      Val a = lower_expr(e.args[0]);
      return emit(e.name == "sum" ? OP_SUM_VEC : OP_LOG_SUM_EXP, {a.slot}, 1);
    }
    if (e.name == "dot_product") {
      Val a = lower_expr(e.args[0]);
      Val b = lower_expr(e.args[1]);
      return emit(OP_DOT, {a.slot, b.slot}, 1);
    }
    fail("unsupported function " + e.name);
  }

  // ---- statements -----------------------------------------------------------
  void lower_read_param(const mir::Stmt& s) {
    int64_t len = 1;
    for (const auto& d : s.read_dims) len *= eval_int(d);
    const int raw = add_slot(len, /*is_param=*/true);
    out.param_names.push_back(s.decl_id);
    out.n_unconstrained += len;

    const mir::Transform& tr = *s.read_transform;
    if (tr.kind == mir::Transform::Identity) {
      scope[s.decl_id] = raw;
      out.views.push_back({s.decl_id, raw, len});
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
      default:
        fail("unsupported parameter transform", tr.raw);
    }
    const int jac = add_slot(1, false);
    Val con = emit(opcode, ins, len, {}, {}, jac);
    jac_slots.push_back(jac);
    scope[s.decl_id] = con.slot;
    out.views.push_back({s.decl_id, con.slot, len});
  }

  void lower_stmt(const mir::Stmt& s) {
    switch (s.kind) {
      case mir::Stmt::Decl:
        if (s.read_transform) {
          lower_read_param(s);
        } else if (s.has_init) {
          scope[s.decl_id] = lower_expr(s.init).slot;
        }
        // Bare decls bind on first assignment.
        return;
      case mir::Stmt::Assignment:
        if (!s.lhs_idx.empty())
          fail("indexed assignment unsupported in M2", s.raw);
        scope[s.lhs] = lower_expr(s.rhs).slot;
        return;
      case mir::Stmt::TargetPE:
        target_terms.push_back(lower_expr(s.target).slot);
        return;
      case mir::Stmt::Block:
      case mir::Stmt::SList:
        for (const auto& k : s.body) lower_stmt(k);
        return;
      case mir::Stmt::Skip:
        return;
      case mir::Stmt::NRFunApp:
        // Compiler-internal checks (FnCheck / FnValidateSize): sizes are
        // enforced at data binding; value checks are skipped in M2.
        if (s.fn_name == "FnCheck" || s.fn_name == "FnValidateSize") return;
        fail("unsupported statement function " + s.fn_name);
      case mir::Stmt::For:
        fail("For loops unsupported in M2 tier-1", s.raw);
      case mir::Stmt::IfElse:
        fail("IfElse unsupported in M2 tier-1", s.raw);
      default:
        fail("unsupported statement", s.raw);
    }
  }

  // Scalar terms reduce through chained ADD_N ops (6-input limit per op).
  int reduce_terms(std::vector<int> terms) {
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
        next.push_back(emit(OP_ADD_N, chunk, 1).slot);
      }
      terms = std::move(next);
    }
    return terms[0];
  }

  CompiledModel run(const mir::Program& p) {
    bind_data(p);
    for (const auto& s : p.log_prob) lower_stmt(s);
    std::vector<int> all = target_terms;
    all.insert(all.end(), jac_slots.begin(), jac_slots.end());
    g.result_slot = reduce_terms(all);
    out.graph = std::move(g);
    return std::move(out);
  }
};

}  // namespace

CompiledModel compile_model(const std::string& tmir_text, const DataMap& data) {
  mir::Program prog = mir::read_program(sexp::parse(tmir_text));
  Lowering lo(data);
  return lo.run(prog);
}

}  // namespace stanrt
