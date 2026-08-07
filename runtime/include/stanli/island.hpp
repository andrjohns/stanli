// A tape island: one op standing in for a region of scalar residue no
// graph pass could vectorize (cross-lane recurrences: HMM forward
// algorithms, state-space updates). The region is compiled once, at load
// time, into a flat register program. Forward runs it on plain doubles --
// one dispatch where the region had thousands. Backward replays it under
// stan-math nested autodiff and harvests the live-ins' adjoints: the same
// var arithmetic CmdStan's generated code runs for the same statements, so
// gradients match by construction.
//
// Registers are mutable cells, one per element of every slot the region
// touches; a len-k slot is k consecutive registers, so Eigen::Map works on
// ranges. Values come only from the forward double pass; the var pass
// exists for its adjoints.
//
// Densities appear only in propto-OFF form (the carver refuses propto):
// with no term-dropping, the instantiation is type-uniform and one
// templated call serves both passes. Propto term-dropping depends on
// argument TYPES (see legacy_fns.cpp's dirichlet note), which would need
// per-mask binding -- out of scope until islands absorb target terms.
#ifndef STANLI_ISLAND_HPP
#define STANLI_ISLAND_HPP

#include <stan/math.hpp>

#include <cstdint>
#include <vector>

namespace stanli {

struct IslandProg {
  enum Code : uint8_t {
    CONST,     // dst = imm
    CONSTR,    // dst[0..len) = pool[a + i]  (fill-backed slot absorbed)
    MOV,       // dst = r[a]
    MOVR,      // dst[0..len) = r[a + i]
    ADD, SUB, MUL, DIV,              // dst = r[a] op r[b]
    NEG, EXP, LOG, SQRT, SQUARE,     // dst = op(r[a])
    INV_LOGIT, LOG1M, TANH,
    LOG_RANGE, EXP_RANGE,            // dst[i] = op(r[a+i]), i < len
    DOT,       // dst = sum_i r[a+i] * r[b+i]      (Eigen redux, as OP_DOT)
    LSE_RANGE, // dst = log_sum_exp(r[a..a+len))
    SOFTMAX,   // dst[0..len) = softmax(r[a..a+len))
    LSE2,      // dst = log_sum_exp(r[a], r[b])
    LOG_MIX,   // dst = log_mix(r[a], r[b], r[c])
    // Densities, propto-OFF only (the carver refuses propto). With no
    // term-dropping the value does not depend on which arguments are
    // autodiff, so binding all of them as T reproduces the scalar op's
    // value exactly; the extra partials computed for data arguments are
    // discarded when the executor hands the island a null adjoint.
    STD_NORMAL,   // 1 arg
    EXPONENTIAL,  // 2 args
    NORMAL,       // 3 args
    LOGNORMAL,
    CAUCHY,
    GAMMA,
    INV_GAMMA,
    BETA,
    WEIBULL,
    LOGISTIC,
    DOUBLE_EXP,
    UNIFORM,
  };
  struct Instr {
    Code code = CONST;
    int32_t dst = 0, a = 0, b = 0, c = 0;
    int32_t len = 0;
    double imm = 0;
  };

  std::vector<Instr> code;
  std::vector<double> pool;  // CONSTR data
  int n_regs = 0;
  // Live-in k seeds registers [ins[k].reg, ins[k].reg + ins[k].len) from
  // the op's ctx.in[k]; the kernel snapshots the same values into scratch
  // so the backward replay is immune to later in-place overwrites.
  struct LiveIn {
    int reg = 0;
    int len = 0;
  };
  std::vector<LiveIn> ins;
  std::vector<int> out_regs;  // packed live-out elements, in out order
};

// Evaluate on T = double (forward) or stan::math::var (backward replay,
// inside the caller's nested_rev_autodiff). The register file is reused
// between calls; the compiler guarantees every register is written before
// read. Not reentrant; islands cannot contain islands.
template <typename T>
void run_island(const IslandProg& p, const T* const* in, T* out) {
  static thread_local std::vector<T> reg;
  if ((int64_t)reg.size() < p.n_regs) reg.resize((size_t)p.n_regs);
  for (size_t k = 0; k < p.ins.size(); ++k)
    for (int i = 0; i < p.ins[k].len; ++i)
      reg[(size_t)(p.ins[k].reg + i)] = in[k][i];

  using VecT = Eigen::Matrix<T, Eigen::Dynamic, 1>;
  const int64_t n = (int64_t)p.code.size();
  for (int64_t pc = 0; pc < n; ++pc) {
    const IslandProg::Instr& I = p.code[(size_t)pc];
    auto d = [&]() -> T& { return reg[(size_t)I.dst]; };
    auto ra = [&]() -> const T& { return reg[(size_t)I.a]; };
    auto rb = [&]() -> const T& { return reg[(size_t)I.b]; };
    switch (I.code) {
      case IslandProg::CONST: d() = T(I.imm); break;
      case IslandProg::CONSTR:
        for (int32_t i = 0; i < I.len; ++i)
          reg[(size_t)(I.dst + i)] = T(p.pool[(size_t)(I.a + i)]);
        break;
      case IslandProg::MOV: d() = ra(); break;
      case IslandProg::MOVR:
        for (int32_t i = 0; i < I.len; ++i)
          reg[(size_t)(I.dst + i)] = reg[(size_t)(I.a + i)];
        break;
      case IslandProg::ADD: d() = ra() + rb(); break;
      case IslandProg::SUB: d() = ra() - rb(); break;
      case IslandProg::MUL: d() = ra() * rb(); break;
      case IslandProg::DIV: d() = ra() / rb(); break;
      case IslandProg::NEG: d() = -ra(); break;
      case IslandProg::EXP: d() = stan::math::exp(ra()); break;
      case IslandProg::LOG: d() = stan::math::log(ra()); break;
      case IslandProg::SQRT: d() = stan::math::sqrt(ra()); break;
      case IslandProg::SQUARE: d() = stan::math::square(ra()); break;
      case IslandProg::INV_LOGIT: d() = stan::math::inv_logit(ra()); break;
      case IslandProg::LOG1M: d() = stan::math::log1m(ra()); break;
      case IslandProg::TANH: d() = stan::math::tanh(ra()); break;
      case IslandProg::LOG_RANGE:
        for (int32_t i = 0; i < I.len; ++i)
          reg[(size_t)(I.dst + i)] = stan::math::log(reg[(size_t)(I.a + i)]);
        break;
      case IslandProg::EXP_RANGE:
        for (int32_t i = 0; i < I.len; ++i)
          reg[(size_t)(I.dst + i)] = stan::math::exp(reg[(size_t)(I.a + i)]);
        break;
      case IslandProg::DOT: {
        Eigen::Map<const VecT> a(&reg[(size_t)I.a], I.len);
        Eigen::Map<const VecT> b(&reg[(size_t)I.b], I.len);
        if constexpr (std::is_same_v<T, double>) {
          // Bitwise-match OP_DOT's kernel: array product, Eigen redux.
          d() = (a.array() * b.array()).sum();
        } else {
          d() = stan::math::dot_product(a, b);
        }
        break;
      }
      case IslandProg::LSE_RANGE: {
        Eigen::Map<const VecT> a(&reg[(size_t)I.a], I.len);
        d() = stan::math::log_sum_exp(a);
        break;
      }
      case IslandProg::SOFTMAX: {
        Eigen::Map<const VecT> a(&reg[(size_t)I.a], I.len);
        const VecT s = stan::math::softmax(a);
        for (int32_t i = 0; i < I.len; ++i) reg[(size_t)(I.dst + i)] = s(i);
        break;
      }
      case IslandProg::LSE2:
        d() = stan::math::log_sum_exp(ra(), rb());
        break;
      case IslandProg::LOG_MIX:
        d() = stan::math::log_mix(ra(), rb(), reg[(size_t)I.c]);
        break;
      case IslandProg::STD_NORMAL:
        d() = stan::math::std_normal_lpdf<false>(ra());
        break;
      case IslandProg::EXPONENTIAL:
        d() = stan::math::exponential_lpdf<false>(ra(), rb());
        break;
      case IslandProg::NORMAL:
        d() = stan::math::normal_lpdf<false>(ra(), rb(), reg[(size_t)I.c]);
        break;
      case IslandProg::LOGNORMAL:
        d() = stan::math::lognormal_lpdf<false>(ra(), rb(), reg[(size_t)I.c]);
        break;
      case IslandProg::CAUCHY:
        d() = stan::math::cauchy_lpdf<false>(ra(), rb(), reg[(size_t)I.c]);
        break;
      case IslandProg::GAMMA:
        d() = stan::math::gamma_lpdf<false>(ra(), rb(), reg[(size_t)I.c]);
        break;
      case IslandProg::INV_GAMMA:
        d() = stan::math::inv_gamma_lpdf<false>(ra(), rb(), reg[(size_t)I.c]);
        break;
      case IslandProg::BETA:
        d() = stan::math::beta_lpdf<false>(ra(), rb(), reg[(size_t)I.c]);
        break;
      case IslandProg::WEIBULL:
        d() = stan::math::weibull_lpdf<false>(ra(), rb(), reg[(size_t)I.c]);
        break;
      case IslandProg::LOGISTIC:
        d() = stan::math::logistic_lpdf<false>(ra(), rb(), reg[(size_t)I.c]);
        break;
      case IslandProg::DOUBLE_EXP:
        d() = stan::math::double_exponential_lpdf<false>(ra(), rb(),
                                                         reg[(size_t)I.c]);
        break;
      case IslandProg::UNIFORM:
        d() = stan::math::uniform_lpdf<false>(ra(), rb(), reg[(size_t)I.c]);
        break;
    }
  }

  for (size_t i = 0; i < p.out_regs.size(); ++i)
    out[i] = reg[(size_t)p.out_regs[i]];
}

struct Graph;  // graph.hpp

// The carver: replace maximal compilable runs of scalar residue with
// OP_ISLAND ops (payload in g.udata_pool) plus one INDEX/SLICE per
// live-out writing the original slot ids. Runs after every other pass.
// fills provides the constant pool for CONSTR absorption; target_terms
// and extra_roots are the slots the pass must not absorb.
// Returns the number of islands carved.
int carve_islands(Graph& g,
                  const std::vector<std::pair<int, std::vector<double>>>& fills,
                  const std::vector<int>& target_terms,
                  const std::vector<int>& extra_roots);

}  // namespace stanli

#endif
