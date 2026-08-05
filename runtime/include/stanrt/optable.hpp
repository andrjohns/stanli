// Opcode registry. Every op, native or legacy, presents the same interface.
#ifndef STANRT_OPTABLE_HPP
#define STANRT_OPTABLE_HPP

#include <stanrt/graph.hpp>

namespace stanrt {

enum Opcode : uint16_t {
  OP_EXP = 1,
  OP_ADD_N,
  OP_BCAST_FMA,
  OP_MATVEC,
  OP_NORMAL_LPDF,
  OP_CAUCHY_LPDF,
  OP_STUDENT_T_LPDF,
  OP_GAMMA_LPDF,
  OP_BETA_LPDF,
  OP_POISSON_LOG_LPMF,
  OP_BERNOULLI_LOGIT_LPMF,
  OP_LOG_SUM_EXP,
  OP_SOFTMAX,
  OP_SUM_VEC,
  OP_CONSTRAIN_LOWER,
  OP_CONSTRAIN_UPPER,
  OP_CONSTRAIN_LU,
  OP_COUNT_
};

struct Kernel {
  // Reads ctx.in values, writes ctx.out, may stash partials in ctx.scratch.
  void (*forward)(KernelCtx&) = nullptr;
  // Reads ctx.out_adj / out_adj_vec (+ values, scratch), accumulates into
  // ctx.in_adj entries whose data is non-null.
  void (*backward)(KernelCtx&) = nullptr;
  // Scratch doubles needed, given bound slot shapes. Null means zero.
  int64_t (*scratch_size)(const Op&, const Slot* slots) = nullptr;
};

Kernel& kernel(uint16_t opcode);
// Called by kernel TUs at static-init time.
void register_kernel(uint16_t opcode, Kernel k);

}  // namespace stanrt

#endif
