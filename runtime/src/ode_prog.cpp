// MIR -> RhsProgram. See runtime/include/stanli/ode_prog.hpp for why.
//
// The shape of the problem makes this much smaller than a general compiler:
// an ODE right-hand side takes (t, y, theta, x_r, x_i) with sizes the
// integrate_ode_* call already fixed, its loops run over those sizes, and its
// integers all come from x_i or from loop variables. So every size, every
// index and every integer is known here, and only the reals need registers.
#include <stanli/ode_prog.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace stanli {
namespace {

// A value is a contiguous run of registers: scalars are runs of one, arrays
// and vectors are runs of their length.
struct Range {
  int reg = 0;
  int len = 0;
};

struct Bail {
  std::string why;
};

struct Compiler {
  RhsProgram& p;
  const std::map<std::string, const mir::FunDef*>& funs;
  std::map<std::string, Range> reals;
  std::map<std::string, std::vector<long>> ints;
  int branch_depth = 0;  // inside a branch on a runtime value
  int inline_depth = 0;

  // Registers are never recycled. Right-hand sides are a few lines over a
  // handful of states, so the count stays in the dozens; the cap is a
  // backstop against a pathological unroll, and trips into the interpreter
  // rather than into a huge allocation.
  static constexpr int kMaxRegs = 1 << 16;

  [[noreturn]] void bail(const std::string& why) { throw Bail{why}; }

  int alloc(int n) {
    if (p.n_regs + n > kMaxRegs) bail("right-hand side needs too many registers");
    const int r = p.n_regs;
    p.n_regs += n;
    return r;
  }

  int emit(RhsProgram::Code c, int dst, int a = 0, int b = 0, double imm = 0) {
    p.code.push_back(RhsProgram::Instr{c, dst, a, b, imm});
    return (int)p.code.size() - 1;
  }

  int konst(double v) {
    const int r = alloc(1);
    emit(RhsProgram::CONST, r, 0, 0, v);
    return r;
  }

  // ---- compile-time integers ----------------------------------------------
  long cint(const mir::Expr& e) {
    switch (e.kind) {
      case mir::Expr::LitInt:
        return e.lit_i;
      case mir::Expr::LitReal:
        return (long)e.lit;
      case mir::Expr::Var: {
        auto it = ints.find(e.name);
        if (it != ints.end() && it->second.size() == 1) return it->second[0];
        bail("integer " + e.name + " is not known at compile time");
      }
      case mir::Expr::Indexed: {
        if (e.args.size() != 2 || e.args[1].name != "IndexSingle")
          bail("integer index form");
        if (e.args[0].kind != mir::Expr::Var) bail("integer index base");
        auto it = ints.find(e.args[0].name);
        if (it == ints.end()) bail("integer array " + e.args[0].name);
        const long ix = cint(e.args[1].args[0]);
        if (ix < 1 || (size_t)ix > it->second.size()) bail("integer index range");
        return it->second[(size_t)ix - 1];
      }
      case mir::Expr::FunApp:
        if (e.args.size() == 2) {
          if (e.name == "Plus__") return cint(e.args[0]) + cint(e.args[1]);
          if (e.name == "Minus__") return cint(e.args[0]) - cint(e.args[1]);
          if (e.name == "Times__") return cint(e.args[0]) * cint(e.args[1]);
          if (e.name == "IntDivide__" || e.name == "Divide__")
            return cint(e.args[0]) / cint(e.args[1]);
        }
        if (e.args.size() == 1 && e.name == "PMinus__")
          return -cint(e.args[0]);
        bail("integer function " + e.name);
      default:
        bail("integer expression");
    }
  }

  bool try_cint(const mir::Expr& e, long* out) {
    try {
      *out = cint(e);
      return true;
    } catch (Bail&) {
      return false;
    }
  }

  // ---- expressions ---------------------------------------------------------
  Range expr(const mir::Expr& e) {
    switch (e.kind) {
      case mir::Expr::LitInt:
        return {konst((double)e.lit_i), 1};
      case mir::Expr::LitReal:
        return {konst(e.lit), 1};
      case mir::Expr::Var: {
        auto it = reals.find(e.name);
        if (it != reals.end()) return it->second;
        auto ii = ints.find(e.name);
        if (ii != ints.end()) {
          const int r = alloc((int)ii->second.size());
          for (size_t k = 0; k < ii->second.size(); ++k)
            emit(RhsProgram::CONST, r + (int)k, 0, 0, (double)ii->second[k]);
          return {r, (int)ii->second.size()};
        }
        bail("unknown variable " + e.name);
      }
      case mir::Expr::Indexed: {
        const Range b = expr(e.args[0]);
        if (e.args.size() == 2 && e.args[1].name == "IndexAll") return b;
        if (e.args.size() != 2 || e.args[1].name != "IndexSingle")
          bail("index form");
        const long ix = cint(e.args[1].args[0]);
        if (ix < 1 || ix > b.len) bail("index out of the declared range");
        return {b.reg + (int)ix - 1, 1};
      }
      case mir::Expr::TernaryIf: {
        long c;
        if (try_cint(e.args[0], &c)) return expr(e.args[c != 0 ? 1 : 2]);
        return branchy_select(e.args[0], e.args[1], e.args[2]);
      }
      case mir::Expr::EOr:
      case mir::Expr::EAnd: {
        // No short-circuit: Stan expressions are total, and the branchless
        // form keeps this out of the instruction stream's way.
        const Range a = expr(e.args[0]), b = expr(e.args[1]);
        if (a.len != 1 || b.len != 1) bail("logical operator on a container");
        const int z = konst(0.0), ta = alloc(1), tb = alloc(1), r = alloc(1);
        emit(RhsProgram::NE, ta, a.reg, z);
        emit(RhsProgram::NE, tb, b.reg, z);
        emit(e.kind == mir::Expr::EOr ? RhsProgram::FMAX : RhsProgram::FMIN, r,
             ta, tb);
        return {r, 1};
      }
      case mir::Expr::FunApp:
        return fun(e);
      default:
        bail("expression");
    }
  }

  // A ternary on a runtime condition: both arms write the same registers.
  Range branchy_select(const mir::Expr& c, const mir::Expr& a,
                       const mir::Expr& b) {
    const Range cv = expr(c);
    if (cv.len != 1) bail("conditional on a container");
    // Compile the arms first to learn the width, then re-emit into place.
    const int jz = emit(RhsProgram::JZ, 0, cv.reg);
    const Range av = expr(a);
    const int dst = alloc(av.len);
    for (int k = 0; k < av.len; ++k)
      emit(RhsProgram::MOV, dst + k, av.reg + k);
    const int jmp = emit(RhsProgram::JMP, 0);
    p.code[(size_t)jz].dst = (int)p.code.size();
    const Range bv = expr(b);
    if (bv.len != av.len) bail("conditional arms of different widths");
    for (int k = 0; k < bv.len; ++k)
      emit(RhsProgram::MOV, dst + k, bv.reg + k);
    p.code[(size_t)jmp].dst = (int)p.code.size();
    return {dst, av.len};
  }

  Range fun(const mir::Expr& e) {
    if (e.fn_lib == mir::Expr::Lib::UserDefined) {
      auto it = funs.find(e.name);
      if (it == funs.end()) bail("unknown function " + e.name);
      std::vector<Range> args;
      std::vector<std::vector<long>> iargs;
      for (const auto& a : e.args) {
        long v;
        if (a.type_ == "UInt" && try_cint(a, &v))
          iargs.push_back({v});
        else
          args.push_back(expr(a));
      }
      return inline_call(*it->second, args, iargs);
    }
    if (e.fn_lib == mir::Expr::Lib::Internal) {
      if (e.name == "FnMakeArray" || e.name == "FnMakeRowVec") {
        std::vector<Range> parts;
        int total = 0;
        for (const auto& a : e.args) {
          parts.push_back(expr(a));
          total += parts.back().len;
        }
        const int r = alloc(total);
        int at = 0;
        for (const Range& q : parts)
          for (int k = 0; k < q.len; ++k)
            emit(RhsProgram::MOV, r + at++, q.reg + k);
        return {r, total};
      }
      bail("internal function " + e.name);
    }
    if (e.args.size() == 2) {
      const Range a = expr(e.args[0]), b = expr(e.args[1]);
      const int n = std::max(a.len, b.len);
      if ((a.len != 1 && a.len != n) || (b.len != 1 && b.len != n))
        bail("binary " + e.name + " on mismatched widths");
      RhsProgram::Code c;
      if (e.name == "Plus__") c = RhsProgram::ADD;
      else if (e.name == "Minus__") c = RhsProgram::SUB;
      else if (e.name == "Times__" || e.name == "EltTimes__") c = RhsProgram::MUL;
      else if (e.name == "Divide__" || e.name == "EltDivide__") c = RhsProgram::DIV;
      else if (e.name == "Pow__" || e.name == "pow") c = RhsProgram::POW;
      else if (e.name == "fmax") c = RhsProgram::FMAX;
      else if (e.name == "fmin") c = RhsProgram::FMIN;
      else if (e.name == "Greater__") c = RhsProgram::GT;
      else if (e.name == "Geq__") c = RhsProgram::GE;
      else if (e.name == "Less__") c = RhsProgram::LT;
      else if (e.name == "Leq__") c = RhsProgram::LE;
      else if (e.name == "Equals__") c = RhsProgram::EQ;
      else if (e.name == "NEquals__") c = RhsProgram::NE;
      else bail("function " + e.name);
      const int r = alloc(n);
      for (int i = 0; i < n; ++i)
        emit(c, r + i, a.reg + (a.len == 1 ? 0 : i),
             b.reg + (b.len == 1 ? 0 : i));
      return {r, n};
    }
    if (e.args.size() == 1) {
      const Range a = expr(e.args[0]);
      if (e.name == "sum") {
        const int r = alloc(1);
        emit(RhsProgram::MOV, r, a.reg);
        for (int i = 1; i < a.len; ++i) emit(RhsProgram::ADD, r, r, a.reg + i);
        return {r, 1};
      }
      RhsProgram::Code c;
      if (e.name == "PMinus__") c = RhsProgram::NEG;
      else if (e.name == "PPlus__") c = RhsProgram::MOV;
      else if (e.name == "exp") c = RhsProgram::EXP;
      else if (e.name == "log") c = RhsProgram::LOG;
      else if (e.name == "sqrt") c = RhsProgram::SQRT;
      else if (e.name == "square") c = RhsProgram::SQUARE;
      else if (e.name == "inv") c = RhsProgram::INV;
      else if (e.name == "fabs" || e.name == "abs") c = RhsProgram::FABS;
      else if (e.name == "inv_logit") c = RhsProgram::INV_LOGIT;
      else bail("function " + e.name);
      const int r = alloc(a.len);
      for (int i = 0; i < a.len; ++i) emit(c, r + i, a.reg + i);
      return {r, a.len};
    }
    bail("function " + e.name);
  }

  // ---- statements ----------------------------------------------------------
  struct Returned {
    Range r;
  };

  int64_t sized_len(const mir::SizedType& t) {
    int64_t n = 1;
    for (const auto& d : t.dims) n *= cint(d);
    return n;
  }

  // Declare (or redeclare) a real variable of `len` registers, zeroed.
  Range declare(const std::string& name, int len) {
    const Range r{alloc(len), len};
    for (int k = 0; k < len; ++k) emit(RhsProgram::CONST, r.reg + k, 0, 0, 0.0);
    reals[name] = r;
    return r;
  }

  void stmt(const mir::Stmt& s) {
    switch (s.kind) {
      case mir::Stmt::Decl: {
        if (s.decl_type.base == "SInt" ||
            (s.decl_type.base == "SArray" && s.decl_type.raw == "SInt")) {
          if (s.has_init) {
            ints[s.decl_id] = {cint(s.init)};
          } else {
            ints[s.decl_id] =
                std::vector<long>((size_t)sized_len(s.decl_type), 0);
          }
          return;
        }
        if (s.has_init) {
          const Range v = expr(s.init);
          const Range d = declare(s.decl_id, v.len);
          for (int k = 0; k < v.len; ++k)
            emit(RhsProgram::MOV, d.reg + k, v.reg + k);
        } else {
          declare(s.decl_id, (int)sized_len(s.decl_type));
        }
        return;
      }
      case mir::Stmt::Assignment: {
        if (ints.count(s.lhs) && s.lhs_idx.empty()) {
          ints[s.lhs] = {cint(s.rhs)};
          return;
        }
        auto it = reals.find(s.lhs);
        if (it == reals.end()) bail("assignment to undeclared " + s.lhs);
        const Range dst = it->second;
        const Range v = expr(s.rhs);
        if (s.lhs_idx.empty()) {
          if (v.len != dst.len) bail("assignment width mismatch for " + s.lhs);
          for (int k = 0; k < v.len; ++k)
            emit(RhsProgram::MOV, dst.reg + k, v.reg + k);
          return;
        }
        if (s.lhs_idx.size() != 1 || s.lhs_idx[0].name != "IndexSingle")
          bail("assignment index form for " + s.lhs);
        const long ix = cint(s.lhs_idx[0].args[0]);
        if (ix < 1 || ix > dst.len) bail("assignment index range for " + s.lhs);
        if (v.len != 1) bail("element assignment from a container");
        emit(RhsProgram::MOV, dst.reg + (int)ix - 1, v.reg);
        return;
      }
      case mir::Stmt::Return:
        // A return under a runtime branch is a control-flow join this flat
        // program has no way to express; the interpreter still handles it.
        if (branch_depth) bail("return inside a data-dependent branch");
        throw Returned{s.has_init ? expr(s.rhs) : Range{0, 0}};
      case mir::Stmt::For: {
        const long lo = cint(s.lower), hi = cint(s.upper);
        for (long v = lo; v <= hi; ++v) {
          ints[s.loopvar] = {v};
          for (const auto& k : s.body) stmt(k);
        }
        ints.erase(s.loopvar);
        return;
      }
      case mir::Stmt::IfElse: {
        long c;
        if (try_cint(s.cond, &c)) {
          if (c != 0 && !s.body.empty()) stmt(s.body[0]);
          if (c == 0 && s.body.size() > 1) stmt(s.body[1]);
          return;
        }
        const Range cv = expr(s.cond);
        if (cv.len != 1) bail("branch on a container");
        ++branch_depth;
        const int jz = emit(RhsProgram::JZ, 0, cv.reg);
        if (!s.body.empty()) stmt(s.body[0]);
        if (s.body.size() > 1) {
          const int jmp = emit(RhsProgram::JMP, 0);
          p.code[(size_t)jz].dst = (int)p.code.size();
          stmt(s.body[1]);
          p.code[(size_t)jmp].dst = (int)p.code.size();
        } else {
          p.code[(size_t)jz].dst = (int)p.code.size();
        }
        --branch_depth;
        return;
      }
      case mir::Stmt::Block:
      case mir::Stmt::SList:
        for (const auto& k : s.body) stmt(k);
        return;
      case mir::Stmt::NRFunApp:
      case mir::Stmt::Skip:
        return;
      default:
        bail("statement");
    }
  }

  Range inline_call(const mir::FunDef& f, const std::vector<Range>& args,
                    const std::vector<std::vector<long>>& iargs) {
    if (++inline_depth > 32) bail("function inlining too deep");
    // Callee scope: save the caller's bindings, install the parameters, and
    // restore afterwards. Registers are never reused, so nothing aliases.
    auto saved_reals = reals;
    auto saved_ints = ints;
    reals.clear();
    ints.clear();
    size_t ai = 0, ii = 0;
    for (size_t k = 0; k < f.arg_names.size(); ++k) {
      const bool is_int = f.arg_types[k].find("UInt") != std::string::npos;
      if (is_int && ii < iargs.size())
        ints[f.arg_names[k]] = iargs[ii++];
      else if (ai < args.size())
        reals[f.arg_names[k]] = args[ai++];
    }
    Range out{0, 0};
    try {
      for (const auto& s : f.body) stmt(s);
      bail("function " + f.name + " returned no value");
    } catch (Returned& r) {
      out = r.r;
    }
    reals = std::move(saved_reals);
    ints = std::move(saved_ints);
    --inline_depth;
    return out;
  }
};

}  // namespace

RhsProgram compile_rhs(const mir::FunDef& f,
                       const std::map<std::string, const mir::FunDef*>& funs,
                       int n_y, int n_theta, int n_x_r,
                       const std::vector<int>& x_i) {
  RhsProgram p;
  if (f.arg_names.size() != 5) {
    p.why = "right-hand side does not take (t, y, theta, x_r, x_i)";
    return p;
  }
  Compiler c{p, funs};
  try {
    // The integrate_ode_* signature fixes the argument order and the sizes.
    p.t_reg = c.alloc(1);
    p.y0 = c.alloc(n_y);
    p.th0 = c.alloc(n_theta);
    p.xr0 = c.alloc(n_x_r);
    p.n_y = n_y;
    p.n_th = n_theta;
    p.n_xr = n_x_r;
    c.reals[f.arg_names[0]] = Range{p.t_reg, 1};
    c.reals[f.arg_names[1]] = Range{p.y0, n_y};
    c.reals[f.arg_names[2]] = Range{p.th0, n_theta};
    c.reals[f.arg_names[3]] = Range{p.xr0, n_x_r};
    c.ints[f.arg_names[4]] = std::vector<long>(x_i.begin(), x_i.end());

    Range out{0, 0};
    try {
      for (const auto& s : f.body) c.stmt(s);
      c.bail("right-hand side returned no value");
    } catch (Compiler::Returned& r) {
      out = r.r;
    }
    if (out.len != n_y)
      c.bail("right-hand side returns " + std::to_string(out.len) +
             " values for " + std::to_string(n_y) + " states");
    for (int k = 0; k < out.len; ++k) p.out_regs.push_back(out.reg + k);
    p.ok = true;
  } catch (Bail& b) {
    p.ok = false;
    p.why = b.why;
    p.code.clear();
    p.out_regs.clear();
  }
  return p;
}

}  // namespace stanli
