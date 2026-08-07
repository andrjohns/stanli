// A compiled ODE right-hand side.
//
// Every other user-defined function is inlined at lowering time. An ODE
// right-hand side cannot be: the integrator picks the times, so the body has
// to stay callable at runtime, on double for the state solve and on var for
// the jacobian stan-math takes at every step. It was therefore evaluated by a
// tree-walking interpreter over the MIR (mir_interp.hpp), which costs a
// std::map lookup per variable reference and a std::vector allocation per
// intermediate -- 5.8 us per call on lotka_volterra's two-line right-hand
// side, against roughly 500 calls per gradient. That interpreter was 97% of
// the model's gradient time.
//
// This compiles the same MIR once, at lowering time, into a flat register
// machine: names become indices, loops with data-known bounds unroll,
// data-only conditions fold away, and evaluation is a switch over a
// contiguous instruction array with no allocation and no lookups. Conditions
// on runtime values (`if (t > 0)`, a dosing schedule) become branches.
//
// Anything it cannot compile leaves `ok` false with a reason, and the caller
// falls back to the interpreter, so coverage never shrinks -- only speed.
#ifndef STANLI_ODE_PROG_HPP
#define STANLI_ODE_PROG_HPP

#include <stanli/mir.hpp>

#include <stan/math.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace stanli {

struct RhsProgram {
  enum Code : uint8_t {
    CONST, MOV,
    ADD, SUB, MUL, DIV, POW, FMAX, FMIN,
    NEG, EXP, LOG, SQRT, SQUARE, INV, FABS, INV_LOGIT,
    GT, GE, LT, LE, EQ, NE,
    JZ,   // jump to `dst` when register `a` is zero
    JMP,  // jump to `dst`
  };
  struct Instr {
    Code code = CONST;
    int32_t dst = 0, a = 0, b = 0;
    double imm = 0;
  };

  std::vector<Instr> code;
  int n_regs = 0;
  // Where run_rhs deposits the call arguments.
  int t_reg = -1, y0 = -1, th0 = -1, xr0 = -1;
  int n_y = 0, n_th = 0, n_xr = 0;
  std::vector<int> out_regs;  // the returned array, in order
  bool ok = false;
  std::string why;  // why not, when !ok
};

// Compile `f` against the argument sizes an integrate_ode_* call fixes.
// Never throws: failure comes back as ok == false.
RhsProgram compile_rhs(const mir::FunDef& f,
                       const std::map<std::string, const mir::FunDef*>& funs,
                       int n_y, int n_theta, int n_x_r,
                       const std::vector<int>& x_i);

// Evaluate. The register file is reused between calls (one per scalar type),
// which is what makes a call allocation-free; the compiler guarantees every
// register is written before it is read, so leftovers are never observed.
// Not reentrant, which is fine: an ODE right-hand side cannot solve an ODE.
template <typename T>
void run_rhs(const RhsProgram& p, const T& t, const T* y, const T* th,
             const double* xr, std::vector<T>& out) {
  static thread_local std::vector<T> reg;
  if ((int)reg.size() < p.n_regs) reg.resize((size_t)p.n_regs);
  reg[(size_t)p.t_reg] = t;
  for (int i = 0; i < p.n_y; ++i) reg[(size_t)(p.y0 + i)] = y[i];
  for (int i = 0; i < p.n_th; ++i) reg[(size_t)(p.th0 + i)] = th[i];
  for (int i = 0; i < p.n_xr; ++i) reg[(size_t)(p.xr0 + i)] = T(xr[i]);

  const int64_t n = (int64_t)p.code.size();
  for (int64_t pc = 0; pc < n; ++pc) {
    const RhsProgram::Instr& I = p.code[(size_t)pc];
    // `dst` is a register for everything but the jumps, where it is an
    // instruction index -- so it is only ever dereferenced inside the cases
    // that actually write a register.
    auto d = [&]() -> T& { return reg[(size_t)I.dst]; };
    switch (I.code) {
      case RhsProgram::CONST: d() = T(I.imm); break;
      case RhsProgram::MOV: d() = reg[(size_t)I.a]; break;
      case RhsProgram::ADD: d() = reg[(size_t)I.a] + reg[(size_t)I.b]; break;
      case RhsProgram::SUB: d() = reg[(size_t)I.a] - reg[(size_t)I.b]; break;
      case RhsProgram::MUL: d() = reg[(size_t)I.a] * reg[(size_t)I.b]; break;
      case RhsProgram::DIV: d() = reg[(size_t)I.a] / reg[(size_t)I.b]; break;
      case RhsProgram::POW:
        d() = stan::math::pow(reg[(size_t)I.a], reg[(size_t)I.b]);
        break;
      case RhsProgram::FMAX:
        d() = stan::math::fmax(reg[(size_t)I.a], reg[(size_t)I.b]);
        break;
      case RhsProgram::FMIN:
        d() = stan::math::fmin(reg[(size_t)I.a], reg[(size_t)I.b]);
        break;
      case RhsProgram::NEG: d() = -reg[(size_t)I.a]; break;
      case RhsProgram::EXP: d() = stan::math::exp(reg[(size_t)I.a]); break;
      case RhsProgram::LOG: d() = stan::math::log(reg[(size_t)I.a]); break;
      case RhsProgram::SQRT: d() = stan::math::sqrt(reg[(size_t)I.a]); break;
      case RhsProgram::SQUARE:
        d() = stan::math::square(reg[(size_t)I.a]);
        break;
      case RhsProgram::INV: d() = stan::math::inv(reg[(size_t)I.a]); break;
      case RhsProgram::FABS: d() = stan::math::fabs(reg[(size_t)I.a]); break;
      case RhsProgram::INV_LOGIT:
        d() = stan::math::inv_logit(reg[(size_t)I.a]);
        break;
      // Comparisons produce a plain 0/1 with no derivative, matching how the
      // generated C++ evaluates them on values.
      case RhsProgram::GT:
        d() = T(stan::math::value_of(reg[(size_t)I.a]) >
              stan::math::value_of(reg[(size_t)I.b]));
        break;
      case RhsProgram::GE:
        d() = T(stan::math::value_of(reg[(size_t)I.a]) >=
              stan::math::value_of(reg[(size_t)I.b]));
        break;
      case RhsProgram::LT:
        d() = T(stan::math::value_of(reg[(size_t)I.a]) <
              stan::math::value_of(reg[(size_t)I.b]));
        break;
      case RhsProgram::LE:
        d() = T(stan::math::value_of(reg[(size_t)I.a]) <=
              stan::math::value_of(reg[(size_t)I.b]));
        break;
      case RhsProgram::EQ:
        d() = T(stan::math::value_of(reg[(size_t)I.a]) ==
              stan::math::value_of(reg[(size_t)I.b]));
        break;
      case RhsProgram::NE:
        d() = T(stan::math::value_of(reg[(size_t)I.a]) !=
              stan::math::value_of(reg[(size_t)I.b]));
        break;
      case RhsProgram::JZ:
        if (stan::math::value_of(reg[(size_t)I.a]) == 0.0) pc = I.dst - 1;
        break;
      case RhsProgram::JMP: pc = I.dst - 1; break;
    }
  }

  out.resize(p.out_regs.size());
  for (size_t i = 0; i < p.out_regs.size(); ++i)
    out[i] = reg[(size_t)p.out_regs[i]];
}

}  // namespace stanli

#endif
