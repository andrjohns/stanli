// Runtime evaluator for a user-defined function body.
//
// The compiler inlines UDFs at lowering time, which is right for everything
// that runs once per gradient. ODE right-hand sides are different: the
// integrator calls them at times chosen during the solve, so the body has to
// be evaluable at runtime on whatever scalar type the integrator hands us
// (double on the forward pass, var inside a nested tape on the reverse one).
//
// This is a small tree-walking interpreter over the same MIR the lowering
// consumes, restricted to what ODE RHS functions actually use: real and int
// locals and arrays, indexing, arithmetic, the common unary math functions,
// array literals, loops, and static branches. Anything outside that raises a
// CompileError naming the construct, exactly like the lowering does, so an
// unsupported RHS is a clear failure and never a silent wrong answer.
#ifndef STANRT_MIR_EVAL_HPP
#define STANRT_MIR_EVAL_HPP

#include <stanrt/compile.hpp>
#include <stanrt/mir.hpp>

#include <stan/math.hpp>

#include <map>
#include <string>
#include <vector>

namespace stanrt {

template <typename T>
class MirEval {
 public:
  using Vec = std::vector<T>;

  MirEval(const std::map<std::string, const mir::FunDef*>& funs)
      : funs_(funs) {}

  // Bind arguments positionally and evaluate the body to its return value.
  Vec call(const mir::FunDef& f, const std::vector<Vec>& args,
           const std::vector<std::vector<int>>& int_args) {
    MirEval sub(funs_);
    size_t ai = 0, ii = 0;
    for (size_t k = 0; k < f.arg_names.size(); ++k) {
      const bool is_int = f.arg_types[k].find("UInt") != std::string::npos;
      if (is_int && ii < int_args.size())
        sub.ints_[f.arg_names[k]] = int_args[ii++];
      else if (ai < args.size())
        sub.reals_[f.arg_names[k]] = args[ai++];
    }
    try {
      for (const auto& s : f.body) sub.exec(s);
    } catch (Return& r) {
      return std::move(r.v);
    }
    fail("ODE function returned no value: " + f.name + " (" +
         std::to_string(f.body.size()) + " statements, " +
         std::to_string(f.arg_names.size()) + " args)");
  }

 private:
  struct Return {
    Vec v;
  };

  [[noreturn]] static void fail(const std::string& msg) {
    throw CompileError("stanrt runtime: " + msg);
  }

  const std::map<std::string, const mir::FunDef*>& funs_;
  std::map<std::string, Vec> reals_;
  std::map<std::string, std::vector<int>> ints_;

  long as_int(const mir::Expr& e) {
    if (e.kind == mir::Expr::LitInt) return e.lit_i;
    if (e.kind == mir::Expr::Var) {
      auto it = ints_.find(e.name);
      if (it != ints_.end() && it->second.size() == 1) return it->second[0];
      auto rt = reals_.find(e.name);
      if (rt != reals_.end() && rt->second.size() == 1)
        return (long)stan::math::value_of(rt->second[0]);
    }
    if (e.kind == mir::Expr::Indexed) {
      Vec b = eval(e.args[0]);
      const long ix = as_int(e.args[1].args[0]);
      return (long)stan::math::value_of(b.at(ix - 1));
    }
    if (e.kind == mir::Expr::FunApp && e.args.size() == 2) {
      const long a = as_int(e.args[0]), b = as_int(e.args[1]);
      if (e.name == "Plus__") return a + b;
      if (e.name == "Minus__") return a - b;
      if (e.name == "Times__") return a * b;
    }
    fail("int expression unsupported in an ODE function");
  }

  Vec eval(const mir::Expr& e) {
    switch (e.kind) {
      case mir::Expr::LitInt:
        return Vec{T(e.lit_i)};
      case mir::Expr::LitReal:
        return Vec{T(e.lit)};
      case mir::Expr::Var: {
        auto it = reals_.find(e.name);
        if (it != reals_.end()) return it->second;
        auto ii = ints_.find(e.name);
        if (ii != ints_.end()) {
          Vec v;
          for (int x : ii->second) v.push_back(T(x));
          return v;
        }
        fail("unknown variable in an ODE function: " + e.name);
      }
      case mir::Expr::Indexed: {
        Vec b = eval(e.args[0]);
        if (e.args.size() != 2 || e.args[1].name != "IndexSingle")
          fail("index form unsupported in an ODE function");
        return Vec{b.at(as_int(e.args[1].args[0]) - 1)};
      }
      case mir::Expr::EOr:
      case mir::Expr::EAnd: {
        const bool a = stan::math::value_of(eval(e.args[0])[0]) != 0.0;
        bool v = a;
        if (e.kind == mir::Expr::EOr ? !a : a)
          v = stan::math::value_of(eval(e.args[1])[0]) != 0.0;
        return Vec{T(v ? 1 : 0)};
      }
      case mir::Expr::TernaryIf:
        return eval(e.args[stan::math::value_of(eval(e.args[0])[0]) != 0.0
                               ? 1
                               : 2]);
      case mir::Expr::FunApp:
        return eval_fun(e);
      default:
        fail("expression unsupported in an ODE function");
    }
  }

  Vec eval_fun(const mir::Expr& e) {
    if (e.fn_lib == mir::Expr::Lib::UserDefined) {
      auto it = funs_.find(e.name);
      if (it == funs_.end()) fail("unknown function: " + e.name);
      std::vector<Vec> args;
      for (const auto& a : e.args) args.push_back(eval(a));
      return call(*it->second, args, {});
    }
    if (e.fn_lib == mir::Expr::Lib::Internal) {
      if (e.name == "FnMakeArray" || e.name == "FnMakeRowVec") {
        Vec out;
        for (const auto& a : e.args) {
          Vec v = eval(a);
          out.insert(out.end(), v.begin(), v.end());
        }
        return out;
      }
      fail("internal function unsupported in an ODE function: " + e.name);
    }
    // Binary and unary stan-library calls, elementwise with broadcasting.
    if (e.args.size() == 2) {
      Vec a = eval(e.args[0]), b = eval(e.args[1]);
      const size_t n = std::max(a.size(), b.size());
      auto at = [](const Vec& v, size_t i) {
        return v.size() == 1 ? v[0] : v.at(i);
      };
      Vec out(n);
      for (size_t i = 0; i < n; ++i) {
        const T x = at(a, i), y = at(b, i);
        if (e.name == "Plus__") out[i] = x + y;
        else if (e.name == "Minus__") out[i] = x - y;
        else if (e.name == "Times__" || e.name == "EltTimes__") out[i] = x * y;
        else if (e.name == "Divide__" || e.name == "EltDivide__")
          out[i] = x / y;
        else if (e.name == "Pow__" || e.name == "pow")
          out[i] = stan::math::pow(x, y);
        else if (e.name == "fmax") out[i] = stan::math::fmax(x, y);
        else if (e.name == "fmin") out[i] = stan::math::fmin(x, y);
        // Comparisons produce plain 0/1 (no derivative), matching how the
        // generated C++ evaluates them on values.
        else if (e.name == "Greater__")
          out[i] = T(stan::math::value_of(x) > stan::math::value_of(y));
        else if (e.name == "Geq__")
          out[i] = T(stan::math::value_of(x) >= stan::math::value_of(y));
        else if (e.name == "Less__")
          out[i] = T(stan::math::value_of(x) < stan::math::value_of(y));
        else if (e.name == "Leq__")
          out[i] = T(stan::math::value_of(x) <= stan::math::value_of(y));
        else if (e.name == "Equals__")
          out[i] = T(stan::math::value_of(x) == stan::math::value_of(y));
        else if (e.name == "NEquals__")
          out[i] = T(stan::math::value_of(x) != stan::math::value_of(y));
        else fail("function unsupported in an ODE function: " + e.name);
      }
      return out;
    }
    if (e.args.size() == 1) {
      Vec a = eval(e.args[0]);
      if (e.name == "sum") {
        T s = T(0);
        for (const T& x : a) s += x;
        return Vec{s};
      }
      Vec out(a.size());
      for (size_t i = 0; i < a.size(); ++i) {
        const T& x = a[i];
        if (e.name == "PMinus__") out[i] = -x;
        else if (e.name == "PPlus__") out[i] = x;
        else if (e.name == "exp") out[i] = stan::math::exp(x);
        else if (e.name == "log") out[i] = stan::math::log(x);
        else if (e.name == "sqrt") out[i] = stan::math::sqrt(x);
        else if (e.name == "square") out[i] = stan::math::square(x);
        else if (e.name == "inv") out[i] = stan::math::inv(x);
        else if (e.name == "fabs" || e.name == "abs")
          out[i] = stan::math::fabs(x);
        else if (e.name == "inv_logit") out[i] = stan::math::inv_logit(x);
        else fail("function unsupported in an ODE function: " + e.name);
      }
      return out;
    }
    fail("function unsupported in an ODE function: " + e.name);
  }

  void exec(const mir::Stmt& s) {
    switch (s.kind) {
      case mir::Stmt::Decl:
        if (s.has_init) {
          if (s.decl_type.base == "SInt")
            ints_[s.decl_id] = {(int)as_int(s.init)};
          else
            reals_[s.decl_id] = eval(s.init);
        } else if (s.decl_type.base == "SInt") {
          ints_[s.decl_id] = {0};
        } else {
          int64_t n = 1;
          for (const auto& d : s.decl_type.dims) n *= as_int(d);
          reals_[s.decl_id] = Vec(n, T(0));
        }
        return;
      case mir::Stmt::Assignment: {
        if (s.lhs_idx.empty()) {
          if (ints_.count(s.lhs))
            ints_[s.lhs] = {(int)as_int(s.rhs)};
          else
            reals_[s.lhs] = eval(s.rhs);
          return;
        }
        if (s.lhs_idx.size() != 1 || s.lhs_idx[0].name != "IndexSingle")
          fail("assignment form unsupported in an ODE function");
        const long ix = as_int(s.lhs_idx[0].args[0]);
        Vec v = eval(s.rhs);
        Vec& dst = reals_[s.lhs];
        if ((size_t)ix > dst.size()) dst.resize(ix, T(0));
        dst[ix - 1] = v.at(0);
        return;
      }
      case mir::Stmt::Return:
        throw Return{s.has_init ? eval(s.rhs) : Vec{}};
      case mir::Stmt::For: {
        const long lo = as_int(s.lower), hi = as_int(s.upper);
        for (long v = lo; v <= hi; ++v) {
          ints_[s.loopvar] = {(int)v};
          for (const auto& k : s.body) exec(k);
        }
        ints_.erase(s.loopvar);
        return;
      }
      case mir::Stmt::IfElse: {
        const bool c = stan::math::value_of(eval(s.cond)[0]) != 0.0;
        if (c && !s.body.empty()) exec(s.body[0]);
        if (!c && s.body.size() > 1) exec(s.body[1]);
        return;
      }
      case mir::Stmt::Block:
      case mir::Stmt::SList:
        for (const auto& k : s.body) exec(k);
        return;
      case mir::Stmt::NRFunApp:
      case mir::Stmt::Skip:
        return;
      default:
        fail("statement unsupported in an ODE function");
    }
  }
};

}  // namespace stanrt

#endif
