#include <stanrt/compile.hpp>
#include <stanrt/mir.hpp>
#include <stanrt/optable.hpp>
#include <stanrt/sexp.hpp>

#include <functional>
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

// Compile-time interpreter for prepare_data: everything there is DataOnly,
// so transformed data evaluates to plain doubles/ints before lowering.
struct TEnv {
  std::map<std::string, DataMap::Entry> vars;

  DataMap::Entry* find(const std::string& n) {
    auto it = vars.find(n);
    return it == vars.end() ? nullptr : &it->second;
  }
};

struct Lowering {
  const DataMap& data;
  TEnv env;
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
                       (raw.empty() ? "" : " | in: " + raw));
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
        if (it != int_env.end()) return it->second;
        DataMap::Entry* en = env.find(e.name);
        if (en && en->is_int && en->i.size() == 1) return en->i[0];
        fail("size expression needs unknown int " + e.name);
      }
      case mir::Expr::Indexed: {
        DataMap::Entry* en = e.args[0].kind == mir::Expr::Var
                                 ? env.find(e.args[0].name)
                                 : nullptr;
        if (en && en->is_int && e.args.size() == 2 &&
            e.args[1].name == "IndexSingle")
          return en->i.at(eval_int(e.args[1].args[0]) - 1);
        if (en && en->is_int && e.args.size() == 3 &&
            e.args[1].name == "IndexSingle" &&
            e.args[2].name == "IndexSingle" && en->dims.size() == 2)
          return en->i.at((eval_int(e.args[2].args[0]) - 1) * en->dims[0] +
                          (eval_int(e.args[1].args[0]) - 1));
        fail("unsupported int index expression", e.raw);
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

  // ---- data block: interpret prepare_data over doubles ----------------------
  DataMap::Entry td_eval(const mir::Expr& e) {
    DataMap::Entry r;
    switch (e.kind) {
      case mir::Expr::LitInt:
        r.is_int = true;
        r.i = {(int)e.lit_i};
        r.r = {(double)e.lit_i};
        return r;
      case mir::Expr::LitReal:
        r.r = {e.lit};
        return r;
      case mir::Expr::Var: {
        DataMap::Entry* en = env.find(e.name);
        if (!en)
          fail("prepare_data: unknown variable " + e.name +
               " (type " + e.type_ + ")", e.raw);
        return *en;
      }
      case mir::Expr::Indexed: {
        DataMap::Entry base = td_eval(e.args[0]);
        if (e.args.size() == 2 && e.args[1].name == "IndexSingle" &&
            base.dims.size() <= 1) {
          const long ix = eval_int_td(e.args[1].args[0]);
          r.is_int = base.is_int;
          if (base.is_int) r.i = {base.i.at(ix - 1)};
          r.r = {base.r.at(ix - 1)};
          return r;
        }
        if (e.args.size() == 2 && e.args[1].name == "IndexSingle" &&
            base.dims.size() == 2) {
          // Row of a 2-D array (col-major storage).
          const long i = eval_int_td(e.args[1].args[0]);
          const int64_t R = base.dims[0], C = base.dims[1];
          r.is_int = base.is_int;
          r.dims = {C};
          for (int64_t j = 0; j < C; ++j) {
            r.r.push_back(base.r.at(j * R + (i - 1)));
            if (base.is_int) r.i.push_back(base.i.at(j * R + (i - 1)));
          }
          return r;
        }
        if (e.args.size() == 2 && e.args[1].name == "IndexAll") return base;
        if (e.args.size() == 2 && e.args[1].name == "IndexSingle" &&
            base.dims.size() == 2) {
          // Row of a 2-D array (col-major storage).
          const long i = eval_int_td(e.args[1].args[0]);
          const int64_t R = base.dims[0], C = base.dims[1];
          r.is_int = base.is_int;
          r.dims = {C};
          for (int64_t j = 0; j < C; ++j) {
            r.r.push_back(base.r.at(j * R + (i - 1)));
            if (base.is_int) r.i.push_back(base.i.at(j * R + (i - 1)));
          }
          return r;
        }
        // Matrix row/element access: [i, j] etc.
        if (e.args.size() == 3 && e.args[1].name == "IndexSingle" &&
            e.args[2].name == "IndexSingle" && base.dims.size() == 2) {
          const long i = eval_int_td(e.args[1].args[0]);
          const long j = eval_int_td(e.args[2].args[0]);
          r.r = {base.r.at((j - 1) * base.dims[0] + (i - 1))};
          return r;
        }
        fail("prepare_data: unsupported index", e.raw);
      }
      case mir::Expr::FunApp: {
        auto bin = [&](auto f) {
          DataMap::Entry a = td_eval(e.args[0]), b = td_eval(e.args[1]);
          DataMap::Entry o;
          const size_t n = std::max(a.r.size(), b.r.size());
          o.r.resize(n);
          for (size_t i = 0; i < n; ++i)
            o.r[i] = f(a.r[a.r.size() == 1 ? 0 : i],
                       b.r[b.r.size() == 1 ? 0 : i]);
          o.dims = a.r.size() >= b.r.size() ? a.dims : b.dims;
          if (a.is_int && b.is_int && a.i.size() == 1 && b.i.size() == 1) {
            o.is_int = true;
            o.i = {(int)f(a.i[0], b.i[0])};
          }
          return o;
        };
        auto un = [&](auto f) {
          DataMap::Entry a = td_eval(e.args[0]);
          DataMap::Entry o;
          o.dims = a.dims;
          o.r.resize(a.r.size());
          for (size_t i = 0; i < a.r.size(); ++i) o.r[i] = f(a.r[i]);
          return o;
        };
        if (e.name == "Plus__") return bin([](double x, double y) { return x + y; });
        if (e.name == "Minus__") return bin([](double x, double y) { return x - y; });
        if (e.name == "Times__" || e.name == "EltTimes__")
          return bin([](double x, double y) { return x * y; });
        if (e.name == "Divide__" || e.name == "EltDivide__")
          return bin([](double x, double y) { return x / y; });
        if (e.name == "Pow__" || e.name == "pow")
          return bin([](double x, double y) { return std::pow(x, y); });
        if (e.name == "PMinus__") return un([](double x) { return -x; });
        if (e.name == "PPlus__") return un([](double x) { return x; });
        if (e.name == "exp") return un([](double x) { return std::exp(x); });
        if (e.name == "log") return un([](double x) { return std::log(x); });
        if (e.name == "log10") return un([](double x) { return std::log10(x); });
        if (e.name == "sqrt") return un([](double x) { return std::sqrt(x); });
        if (e.name == "square") return un([](double x) { return x * x; });
        if (e.name == "fabs" || e.name == "abs")
          return un([](double x) { return std::fabs(x); });
        if (e.name == "mean") {
          DataMap::Entry a = td_eval(e.args[0]);
          double m = 0;
          for (double v : a.r) m += v;
          r.r = {m / (double)a.r.size()};
          return r;
        }
        if (e.name == "sd") {
          DataMap::Entry a = td_eval(e.args[0]);
          double m = 0;
          for (double v : a.r) m += v;
          m /= (double)a.r.size();
          double s2 = 0;
          for (double v : a.r) s2 += (v - m) * (v - m);
          r.r = {std::sqrt(s2 / (double)(a.r.size() - 1))};
          return r;
        }
        if (e.name == "sum") {
          DataMap::Entry a = td_eval(e.args[0]);
          double m = 0;
          for (double v : a.r) m += v;
          r.r = {m};
          return r;
        }
        if (e.name == "rep_vector" || e.name == "rep_row_vector") {
          DataMap::Entry a = td_eval(e.args[0]);
          const long n = eval_int_td(e.args[1]);
          r.r.assign(n, a.r.at(0));
          r.dims = {n};
          return r;
        }
        if (e.name == "Equals__") return bin([](double x, double y) { return x == y ? 1.0 : 0.0; });
        if (e.name == "NEquals__") return bin([](double x, double y) { return x != y ? 1.0 : 0.0; });
        if (e.name == "Greater__") return bin([](double x, double y) { return x > y ? 1.0 : 0.0; });
        if (e.name == "Geq__") return bin([](double x, double y) { return x >= y ? 1.0 : 0.0; });
        if (e.name == "Less__") return bin([](double x, double y) { return x < y ? 1.0 : 0.0; });
        if (e.name == "Leq__") return bin([](double x, double y) { return x <= y ? 1.0 : 0.0; });
        if (e.name == "PNot__") return un([](double x) { return x == 0.0 ? 1.0 : 0.0; });
        if (e.name == "max" || e.name == "min") {
          DataMap::Entry a = td_eval(e.args[0]);
          if (e.args.size() == 2) {
            DataMap::Entry b = td_eval(e.args[1]);
            const double m = e.name == "max"
                                 ? std::max(a.r.at(0), b.r.at(0))
                                 : std::min(a.r.at(0), b.r.at(0));
            r.is_int = a.is_int && b.is_int;
            if (r.is_int) r.i = {(int)m};
            r.r = {m};
            return r;
          }
          double m = a.r.at(0);
          for (double x : a.r) m = e.name == "max" ? std::max(m, x) : std::min(m, x);
          r.is_int = a.is_int;
          if (r.is_int) r.i = {(int)m};
          r.r = {m};
          return r;
        }
        if (e.name == "FnMakeArray" || e.name == "FnMakeRowVec") {
          DataMap::Entry o;
          o.is_int = true;
          bool rows_mode = false;
          int64_t row_len = 0;
          for (const auto& a : e.args) {
            DataMap::Entry v2 = td_eval(a);
            if (v2.r.size() > 1 || rows_mode) {
              // Row-vector elements: build a matrix, row-major.
              rows_mode = true;
              row_len = (int64_t)v2.r.size();
              o.is_int = false;
              o.r.insert(o.r.end(), v2.r.begin(), v2.r.end());
              continue;
            }
            o.r.push_back(v2.r.at(0));
            if (v2.is_int && !v2.i.empty()) o.i.push_back(v2.i[0]);
            else o.is_int = false;
          }
          if (!o.is_int) o.i.clear();
          if (rows_mode) {
            // Rows arrived row-by-row; store column-major.
            const int64_t R = (int64_t)e.args.size(), C = row_len;
            std::vector<double> cm(R * C);
            for (int64_t i = 0; i < R; ++i)
              for (int64_t j = 0; j < C; ++j)
                cm[j * R + i] = o.r[i * C + j];
            o.r = std::move(cm);
          }
          if (rows_mode)
            o.dims = {(int64_t)e.args.size(), row_len};
          else
            o.dims = {(int64_t)o.r.size()};
          return o;
        }
        if (e.name == "Transpose__") {
          DataMap::Entry a = td_eval(e.args[0]);
          if (a.dims.size() < 2) return a;  // vector transpose: same storage
          DataMap::Entry o;
          o.dims = {a.dims[1], a.dims[0]};
          o.r.resize(a.r.size());
          // col-major: o(j,i) = a(i,j)
          for (int64_t i = 0; i < a.dims[0]; ++i)
            for (int64_t j = 0; j < a.dims[1]; ++j)
              o.r[i * a.dims[1] + j] = a.r[j * a.dims[0] + i];
          return o;
        }
        if (e.name == "to_vector") {
          DataMap::Entry a = td_eval(e.args[0]);
          a.dims = {(int64_t)a.r.size()};
          a.is_int = false;
          return a;
        }
        fail("prepare_data: unsupported function " + e.name, e.raw);
      }
      default:
        fail("prepare_data: unsupported expression", e.raw);
    }
  }

  long eval_int_td(const mir::Expr& e) {
    DataMap::Entry v = td_eval(e);
    if (v.is_int && v.i.size() == 1) return v.i[0];
    if (v.r.size() == 1) return (long)v.r[0];
    fail("prepare_data: expected int scalar", e.raw);
  }

  void td_exec(const mir::Stmt& st) {
    switch (st.kind) {
      case mir::Stmt::Decl: {
        DataMap::Entry e;
        if (st.decl_type.base == "SInt" ||
            (st.decl_type.base == "SArray" && st.decl_type.raw == "SInt"))
          e.is_int = true;
        if (st.has_init && st.init.kind == mir::Expr::FunApp &&
            st.init.fn_lib == mir::Expr::Lib::Internal &&
            st.init.name == "FnReadData") {
          // Reads name the source data variable in their argument.
          e = data.at(st.init.args.at(0).lit_s);
        } else if (st.has_init &&
                   !(st.init.kind == mir::Expr::FunApp &&
                     st.init.fn_lib == mir::Expr::Lib::Internal)) {
          e = td_eval(st.init);
        } else if (data.has(st.decl_id)) {
          e = data.at(st.decl_id);
        } else if (!st.decl_type.base.empty() &&
                   st.decl_type.base != "SInt" &&
                   st.decl_type.base != "SReal") {
          // Bare sized decl: allocate zeros so element writes work.
          int64_t n = 1;
          std::vector<int64_t> dims;
          for (const auto& d : st.decl_type.dims) {
            const long v = eval_int_td(d);
            dims.push_back(v);
            n *= v;
          }
          e.r.assign(n, 0.0);
          if (e.is_int) e.i.assign(n, 0);
          e.dims = std::move(dims);
        }
        env.vars[st.decl_id] = std::move(e);
        return;
      }
      case mir::Stmt::Assignment: {
        // Data reads: the FnReadData argument names the source variable.
        std::string read_name;
        std::function<void(const mir::Expr&)> scan = [&](const mir::Expr& x) {
          if (x.kind == mir::Expr::FunApp && x.name == "FnReadData" &&
              !x.args.empty())
            read_name = x.args[0].lit_s;
          for (const auto& a : x.args) scan(a);
        };
        scan(st.rhs);
        if (!read_name.empty()) {
          // The flat read buffer is consumed with sequential 1-D indexing
          // regardless of the source variable's shape.
          DataMap::Entry flat = data.at(read_name);
          flat.dims = {(int64_t)std::max(flat.r.size(), flat.i.size())};
          env.vars[st.lhs] = std::move(flat);
          return;
        }
        if (st.lhs_idx.empty()) {
          env.vars[st.lhs] = td_eval(st.rhs);
          return;
        }
        DataMap::Entry* en = env.find(st.lhs);
        if (!en) fail("prepare_data: assignment to unknown " + st.lhs);
        DataMap::Entry v = td_eval(st.rhs);
        if (st.lhs_idx.size() == 1 && st.lhs_idx[0].name == "IndexSingle") {
          const long ix = eval_int_td(st.lhs_idx[0].args[0]);
          if ((size_t)ix > en->r.size()) en->r.resize(ix, 0.0);
          en->r[ix - 1] = v.r.at(0);
          if (en->is_int) {
            if ((size_t)ix > en->i.size()) en->i.resize(ix, 0);
            en->i[ix - 1] = v.is_int && !v.i.empty() ? v.i[0]
                                                     : (int)v.r.at(0);
          }
          return;
        }
        if (st.lhs_idx.size() == 2 && st.lhs_idx[0].name == "IndexSingle" &&
            st.lhs_idx[1].name == "IndexSingle" && en->dims.size() == 2) {
          const long i = eval_int_td(st.lhs_idx[0].args[0]);
          const long j = eval_int_td(st.lhs_idx[1].args[0]);
          const int64_t flat = (j - 1) * en->dims[0] + (i - 1);
          en->r.at(flat) = v.r.at(0);
          if (en->is_int) en->i.at(flat) = v.is_int && !v.i.empty()
                                               ? v.i[0]
                                               : (int)v.r.at(0);
          return;
        }
        fail("prepare_data: unsupported indexed assignment");
      }
      case mir::Stmt::For: {
        const long lo = eval_int_td(st.lower), hi = eval_int_td(st.upper);
        for (long v = lo; v <= hi; ++v) {
          DataMap::Entry lv;
          lv.is_int = true;
          lv.i = {(int)v};
          lv.r = {(double)v};
          env.vars[st.loopvar] = lv;
          for (const auto& k : st.body) td_exec(k);
        }
        env.vars.erase(st.loopvar);
        return;
      }
      case mir::Stmt::IfElse: {
        const bool c = td_eval(st.cond).r.at(0) != 0.0;
        if (c && !st.body.empty()) td_exec(st.body[0]);
        if (!c && st.body.size() > 1) td_exec(st.body[1]);
        return;
      }
      case mir::Stmt::Block:
      case mir::Stmt::SList:
        for (const auto& k : st.body) td_exec(k);
        return;
      case mir::Stmt::NRFunApp:
      case mir::Stmt::Skip:
        return;  // checks skipped in M2
      default:
        fail("prepare_data: unsupported statement", st.raw);
    }
  }

  void bind_data(const mir::Program& p) {
    for (const auto& [name, type] : p.input_vars) {
      (void)type;
      if (data.has(name)) env.vars[name] = data.at(name);
    }
    for (const auto& st : p.prepare_data) td_exec(st);
    for (auto& [name, e] : env.vars) {
      if (e.is_int && e.i.size() == 1 && e.dims.empty())
        int_env[name] = e.i[0];
    }
  }

  // Lazily materialize an env value as a data slot when log_prob uses it.
  int env_slot(const std::string& name) {
    DataMap::Entry* en = env.find(name);
    if (!en || en->r.empty()) return -1;
    SlotInfo si;
    si.data_like = true;
    if (en->dims.size() == 2) {
      si.rows = en->dims[0];
      si.cols = en->dims[1];
    }
    const int s = add_slot((int64_t)en->r.size(), false, si);
    out.fills.emplace_back(s, en->r);
    scope[name] = s;
    return s;
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
          const int s = env_slot(e.name);
          if (s >= 0) return {s, info[s]};
          fail("unknown variable " + e.name);
        }
        return {it->second, info[it->second]};
      }
      case mir::Expr::Indexed: {
        // All-Single indices with compile-time values -> element read.
        Val base = lower_expr(e.args[0]);
        if (e.args.size() == 2 && e.args[1].name == "IndexAll") return base;
        int64_t flat = 0;
        if (e.args.size() == 2 && e.args[1].name == "IndexSingle") {
          flat = eval_int(e.args[1].args[0]) - 1;
        } else if (e.args.size() == 3 && e.args[1].name == "IndexSingle" &&
                   e.args[2].name == "IndexSingle" && base.si.rows > 0) {
          flat = (eval_int(e.args[2].args[0]) - 1) * base.si.rows +
                 (eval_int(e.args[1].args[0]) - 1);
        } else {
          fail("unsupported index expression", e.raw);
        }
        return emit(OP_INDEX, {base.slot}, 1, {}, {(int)flat});
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
        {"dirichlet_lpdf", {OP_DIRICHLET_LPDF, 2, 0}},
    };
    auto dit = kDens.find(e.name);
    if (dit != kDens.end()) {
      const Dens& d = dit->second;
      if ((int)e.args.size() != d.nargs)
        fail(e.name + ": expected " + std::to_string(d.nargs) + " args");
      auto int_arg = [&](const mir::Expr& oc) -> std::vector<int> {
        if (oc.kind == mir::Expr::Var) {
          DataMap::Entry* en = env.find(oc.name);
          if (en && en->is_int && !en->i.empty()) return en->i;
          if (int_env.count(oc.name))
            return {static_cast<int>(int_env[oc.name])};
        }
        if (oc.kind == mir::Expr::LitInt)
          return {static_cast<int>(oc.lit_i)};
        if (oc.kind == mir::Expr::Indexed || oc.kind == mir::Expr::FunApp) {
          // Compile-time int expression (e.g. y[n] under an unrolled loop).
          return {static_cast<int>(eval_int(oc))};
        }
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
        {"EltDivide__", OP_DIV}, {"Pow__", OP_POW}, {"pow", OP_POW},
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
    if (e.name == "Transpose__") {
      Val a = lower_expr(e.args[0]);
      if (a.si.rows == 0) return a;  // vector transpose: same flat storage
      fail("matrix transpose unsupported in M2");
    }
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
      if (e.name == "log_sum_exp" && e.args.size() == 2) {
        Val a = lower_expr(e.args[0]);
        Val b = lower_expr(e.args[1]);
        return emit(OP_LSE2, {a.slot, b.slot}, 1);
      }
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
    // Declared (constrained) size from the read dims; the unconstrained raw
    // size depends on the transform (simplex uses K-1).
    int64_t con_len = 1;
    for (const auto& d : s.read_dims) con_len *= eval_int(d);
    const mir::Transform& tr = *s.read_transform;
    int64_t raw_len = con_len;
    if (tr.kind == mir::Transform::Simplex) raw_len = con_len - 1;
    const int raw = add_slot(raw_len, /*is_param=*/true);
    out.param_names.push_back(s.decl_id);
    out.n_unconstrained += raw_len;

    if (tr.kind == mir::Transform::Identity) {
      scope[s.decl_id] = raw;
      out.views.push_back({s.decl_id, raw, raw_len});
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
      case mir::Transform::Simplex:
        opcode = OP_CONSTRAIN_SIMPLEX;
        break;
      case mir::Transform::Ordered:
        opcode = OP_CONSTRAIN_ORDERED;
        break;
      case mir::Transform::PositiveOrdered:
        opcode = OP_CONSTRAIN_POS_ORDERED;
        break;
      default:
        fail("unsupported parameter transform", tr.raw);
    }
    const int jac = add_slot(1, false);
    Val con = emit(opcode, ins, con_len, {}, {}, jac);
    jac_slots.push_back(jac);
    scope[s.decl_id] = con.slot;
    out.views.push_back({s.decl_id, con.slot, con_len});
  }

  struct DeclShape {
    int64_t len = 0, rows = 0, cols = 0;
  };
  std::map<std::string, DeclShape> decl_lens;

  void lower_stmt(const mir::Stmt& s) {
    switch (s.kind) {
      case mir::Stmt::Decl:
        if (s.read_transform) {
          lower_read_param(s);
        } else if (s.has_init) {
          scope[s.decl_id] = lower_expr(s.init).slot;
        } else {
          DeclShape sh;
          sh.len = sized_len(s.decl_type, &sh.rows, &sh.cols);
          decl_lens[s.decl_id] = sh;
        }
        return;
      case mir::Stmt::Assignment: {
        if (!s.lhs_idx.empty()) {
          // Element write under unrolled control flow: functional update.
          int prev;
          auto it = scope.find(s.lhs);
          if (it != scope.end()) {
            prev = it->second;
          } else {
            auto dl = decl_lens.find(s.lhs);
            if (dl == decl_lens.end())
              fail("indexed assignment to undeclared " + s.lhs);
            SlotInfo si;
            si.data_like = true;
            si.rows = dl->second.rows;
            si.cols = dl->second.cols;
            prev = add_slot(dl->second.len, false, si);
            out.fills.emplace_back(
                prev, std::vector<double>(dl->second.len, 0.0));
          }
          int64_t flat = 0;
          if (s.lhs_idx.size() == 1 &&
              s.lhs_idx[0].name == "IndexSingle") {
            flat = eval_int(s.lhs_idx[0].args[0]) - 1;
          } else if (s.lhs_idx.size() == 2 &&
                     s.lhs_idx[0].name == "IndexSingle" &&
                     s.lhs_idx[1].name == "IndexSingle" &&
                     info[prev].rows > 0) {
            flat = (eval_int(s.lhs_idx[1].args[0]) - 1) * info[prev].rows +
                   (eval_int(s.lhs_idx[0].args[0]) - 1);
          } else {
            fail("unsupported indexed assignment", s.raw);
          }
          const int rhs = lower_expr(s.rhs).slot;
          Val nv = emit(OP_SET_INDEX, {prev, rhs}, info[prev].len, info[prev],
                        {(int)flat});
          scope[s.lhs] = nv.slot;
          return;
        }
        scope[s.lhs] = lower_expr(s.rhs).slot;
        return;
      }
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
      case mir::Stmt::For: {
        const long lo = eval_int(s.lower), hi = eval_int(s.upper);
        for (long v = lo; v <= hi; ++v) {
          int_env[s.loopvar] = v;
          for (const auto& k : s.body) lower_stmt(k);
        }
        int_env.erase(s.loopvar);
        return;
      }
      case mir::Stmt::IfElse: {
        if (!s.cond.data_only)
          fail("IfElse on parameters unsupported in M2", s.raw);
        const bool c = td_eval(s.cond).r.at(0) != 0.0;
        if (c && !s.body.empty()) lower_stmt(s.body[0]);
        if (!c && s.body.size() > 1) lower_stmt(s.body[1]);
        return;
      }
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
