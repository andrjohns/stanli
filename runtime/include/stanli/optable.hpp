// Opcode registry. Every op, native or legacy, presents the same interface.
#ifndef STANLI_OPTABLE_HPP
#define STANLI_OPTABLE_HPP

#include <stanli/graph.hpp>

namespace stanli {

// One list, two uses: the enum and the name table are generated from it,
// so a new op cannot be added to one and forgotten in the other.
#define STANLI_OPCODE_LIST(X)                                             \
  X(OP_EXP)                                                               \
  X(OP_ADD_N)                                                             \
  X(OP_BCAST_FMA)                                                         \
  X(OP_MATVEC)                                                            \
  X(OP_POISSON_LOG_LPMF)                                                  \
  X(OP_BERNOULLI_LOGIT_LPMF)                                              \
  X(OP_BERNOULLI_LPMF)                                                    \
  X(OP_POISSON_LPMF)                                                      \
  X(OP_NEG_BINOMIAL_2_LPMF)                                               \
  X(OP_BINOMIAL_LPMF)                                                     \
  X(OP_BINOMIAL_LOGIT_LPMF)                                               \
  X(OP_BERNOULLI_LOGIT_GLM_LPMF)                                          \
  X(OP_LOGIT)                                                             \
  X(OP_MEAN)                                                              \
  X(OP_REP_VEC)                                                           \
  X(OP_INDEX)                                                             \
  X(OP_SET_INDEX)                                                         \
  X(OP_SET_INDEX_INPLACE)                                                 \
  X(OP_SLICE)                                                             \
  X(OP_SET_SLICE)                                                         \
  X(OP_SET_SLICE_STRIDED)                                                 \
  X(OP_SLICE_STRIDED)                                                     \
  X(OP_GATHER)                                                            \
  X(OP_CONCAT2)                                                           \
  X(OP_REP_MAT)                                                           \
  X(OP_GP_EXP_QUAD_COV)                                                   \
  X(OP_DIAG_MATRIX)                                                       \
  X(OP_CHOLESKY)                                                          \
  X(OP_MULTI_NORMAL_CHOL_LPDF)                                            \
  X(OP_MULTI_NORMAL_LPDF)                                                 \
  X(OP_GEMM)                                                              \
  X(OP_LOG_INV_LOGIT)                                                     \
  X(OP_LOG_SOFTMAX)                                                       \
  X(OP_LOG1M_INV_LOGIT)                                                   \
  X(OP_CONSTRAIN_CHOL_CORR)                                               \
  X(OP_LKJ_CORR_CHOL_LPDF)                                                \
  X(OP_NORMAL_ID_GLM_LPDF)                                                \
  X(OP_TRANSPOSE)                                                         \
  X(OP_ODE)                                                               \
  X(OP_ISLAND)                                                            \
  X(OP_EIGENVALUES_SYM)                                                   \
  X(OP_EIGENVECTORS_SYM)                                                  \
  X(OP_LOG_SUM_EXP)                                                       \
  X(OP_LSE2)                                                              \
  X(OP_LOG_MIX)                                                           \
  X(OP_SOFTMAX)                                                           \
  X(OP_SUM_VEC)                                                           \
  X(OP_ADD)                                                               \
  X(OP_SUB)                                                               \
  X(OP_MUL)                                                               \
  X(OP_DIV)                                                               \
  X(OP_POW)                                                               \
  X(OP_DOT)                                                               \
  X(OP_NEG)                                                               \
  X(OP_EXPV)                                                              \
  X(OP_LOGV)                                                              \
  X(OP_INV_LOGIT)                                                         \
  X(OP_SQRT)                                                              \
  X(OP_SQUARE)                                                            \
  X(OP_LOG1M)                                                             \
  X(OP_TANHV)                                                             \
  X(OP_CUMSUM)                                                            \
  X(OP_CONSTRAIN_LOWER)                                                   \
  X(OP_CONSTRAIN_UPPER)                                                   \
  X(OP_CONSTRAIN_LU)                                                      \
  X(OP_CONSTRAIN_SIMPLEX)                                                 \
  X(OP_CONSTRAIN_ORDERED)                                                 \
  X(OP_CONSTRAIN_POS_ORDERED)                                             \
  X(OP_DIRICHLET_LPDF)

// Scalar densities, one line each: this list generates the opcode, the
// name, the kernel, its registration, and the lowering table entry
// (densities.cpp, lower.cpp). They are all the same shape --
// density_fwd_v<N> over stan-math's propto-true/false pair, with
// density_bwd<N> contracting the stashed partials -- so the only
// things that vary are the stan-math name and the argument count.
// Discrete densities are not here: an integer outcome needs idata
// plumbing that differs per distribution.
#define STANLI_SCALAR_DENSITY_LIST(X)                                     \
  X(OP_NORMAL_LPDF, normal_lpdf, 3)                                      \
  X(OP_CAUCHY_LPDF, cauchy_lpdf, 3)                                      \
  X(OP_STUDENT_T_LPDF, student_t_lpdf, 4)                                \
  X(OP_GAMMA_LPDF, gamma_lpdf, 3)                                        \
  X(OP_BETA_LPDF, beta_lpdf, 3)                                          \
  X(OP_LOGNORMAL_LPDF, lognormal_lpdf, 3)                                \
  X(OP_UNIFORM_LPDF, uniform_lpdf, 3)                                    \
  X(OP_DOUBLE_EXP_LPDF, double_exponential_lpdf, 3)                      \
  X(OP_EXPONENTIAL_LPDF, exponential_lpdf, 2)                            \
  X(OP_INV_GAMMA_LPDF, inv_gamma_lpdf, 3)                                \
  X(OP_STD_NORMAL_LPDF, std_normal_lpdf, 1)                              \
  X(OP_WEIBULL_LPDF, weibull_lpdf, 3)                                    \
  X(OP_LOGISTIC_LPDF, logistic_lpdf, 3)                                  \
  X(OP_CHI_SQUARE_LPDF, chi_square_lpdf, 2)                              \
  X(OP_INV_CHI_SQUARE_LPDF, inv_chi_square_lpdf, 2)                      \
  X(OP_SCALED_INV_CHI_SQUARE_LPDF, scaled_inv_chi_square_lpdf, 3)        \
  X(OP_FRECHET_LPDF, frechet_lpdf, 3)                                    \
  X(OP_GUMBEL_LPDF, gumbel_lpdf, 3)                                      \
  X(OP_LOGLOGISTIC_LPDF, loglogistic_lpdf, 3)                            \
  X(OP_PARETO_LPDF, pareto_lpdf, 3)                                      \
  X(OP_PARETO_TYPE_2_LPDF, pareto_type_2_lpdf, 4)                        \
  X(OP_RAYLEIGH_LPDF, rayleigh_lpdf, 2)                                  \
  X(OP_SKEW_NORMAL_LPDF, skew_normal_lpdf, 4)                            \
  X(OP_VON_MISES_LPDF, von_mises_lpdf, 3)                                \
  X(OP_EXP_MOD_NORMAL_LPDF, exp_mod_normal_lpdf, 4)                      \
  X(OP_BETA_PROPORTION_LPDF, beta_proportion_lpdf, 3)                    \
  X(OP_SKEW_DOUBLE_EXPONENTIAL_LPDF, skew_double_exponential_lpdf, 4)

enum Opcode : uint16_t {
  OP_NONE_ = 0,
#define STANLI_OPCODE_ENUM(name) name,
  STANLI_OPCODE_LIST(STANLI_OPCODE_ENUM)
#undef STANLI_OPCODE_ENUM
#define STANLI_DENSITY_ENUM(code, fn, n) code,
  STANLI_SCALAR_DENSITY_LIST(STANLI_DENSITY_ENUM)
#undef STANLI_DENSITY_ENUM
  OP_COUNT_
};

// "OP_NORMAL_LPDF" for a known opcode, "OP_?" otherwise. Diagnostics and
// tooling only; never on a hot path.
const char* opcode_name(uint16_t opcode);

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

}  // namespace stanli

#endif
