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
//
// The last field is whether to instantiate the full activity-mask
// dispatch. A density instantiates stan-math's template once per mask,
// twice for propto and again for the elementwise form -- 4 * 2^N, about
// 630 KB of object each, which is what a precompiled library costs when
// CmdStan gets to instantiate only the combination your model uses.
//
// With propto OFF no terms are dropped, so the value is the same for
// every mask and one all-active binding would do. That is not free: it
// binds data arguments as recorder scalars, so their partials get
// computed and thrown away, and on radon_pooled -- a vectorized normal
// over 919 data points -- it cost 30-40%. The densities models actually
// use keep the dispatch (1); the long tail takes the smaller code (0),
// where the same cost lands on a density nobody has profiled anyway.
#define STANLI_SCALAR_DENSITY_LIST(X)                                     \
  X(OP_NORMAL_LPDF, normal_lpdf, 3, 1)                                      \
  X(OP_CAUCHY_LPDF, cauchy_lpdf, 3, 1)                                      \
  X(OP_STUDENT_T_LPDF, student_t_lpdf, 4, 1)                                \
  X(OP_GAMMA_LPDF, gamma_lpdf, 3, 1)                                        \
  X(OP_BETA_LPDF, beta_lpdf, 3, 1)                                          \
  X(OP_LOGNORMAL_LPDF, lognormal_lpdf, 3, 1)                                \
  X(OP_UNIFORM_LPDF, uniform_lpdf, 3, 1)                                    \
  X(OP_DOUBLE_EXP_LPDF, double_exponential_lpdf, 3, 1)                      \
  X(OP_EXPONENTIAL_LPDF, exponential_lpdf, 2, 1)                            \
  X(OP_INV_GAMMA_LPDF, inv_gamma_lpdf, 3, 1)                                \
  X(OP_STD_NORMAL_LPDF, std_normal_lpdf, 1, 1)                              \
  X(OP_WEIBULL_LPDF, weibull_lpdf, 3, 1)                                    \
  X(OP_LOGISTIC_LPDF, logistic_lpdf, 3, 1)                                  \
  X(OP_CHI_SQUARE_LPDF, chi_square_lpdf, 2, 0)                              \
  X(OP_INV_CHI_SQUARE_LPDF, inv_chi_square_lpdf, 2, 0)                      \
  X(OP_SCALED_INV_CHI_SQUARE_LPDF, scaled_inv_chi_square_lpdf, 3, 0)        \
  X(OP_FRECHET_LPDF, frechet_lpdf, 3, 0)                                    \
  X(OP_GUMBEL_LPDF, gumbel_lpdf, 3, 0)                                      \
  X(OP_LOGLOGISTIC_LPDF, loglogistic_lpdf, 3, 0)                            \
  X(OP_PARETO_LPDF, pareto_lpdf, 3, 0)                                      \
  X(OP_PARETO_TYPE_2_LPDF, pareto_type_2_lpdf, 4, 0)                        \
  X(OP_RAYLEIGH_LPDF, rayleigh_lpdf, 2, 0)                                  \
  X(OP_SKEW_NORMAL_LPDF, skew_normal_lpdf, 4, 0)                            \
  X(OP_VON_MISES_LPDF, von_mises_lpdf, 3, 0)                                \
  X(OP_EXP_MOD_NORMAL_LPDF, exp_mod_normal_lpdf, 4, 0)                      \
  X(OP_BETA_PROPORTION_LPDF, beta_proportion_lpdf, 3, 0)                    \
  X(OP_SKEW_DOUBLE_EXPONENTIAL_LPDF, skew_double_exponential_lpdf, 4, 0)

// Scalar unary math, one line each: opcode, kernel, registration,
// lowering entry and interpreter branch all come from here. The value and
// the derivative are the only things that vary, and `x` is the argument.
//
// These are cheap in a way densities are not. A density instantiates
// stan-math's template once per activity mask, twice for propto and again
// for the elementwise form -- 4 * 2^N per distribution, about 630 KB of
// object each. An entry here costs about 5 KB, because the derivative is
// written out rather than obtained by instantiating an autodiff template.
// fn_sweep.py checks every one against CmdStan, which is what makes
// hand-written derivatives safe to write at this rate.
#define STANLI_SCALAR_UNARY_LIST(X)                                       \
  X(OP_LGAMMA, lgamma, stan::math::lgamma(x), stan::math::digamma(x))  \
  X(OP_DIGAMMA, digamma, stan::math::digamma(x), stan::math::trigamma(x))  \
  X(OP_LOG1P, log1p, stan::math::log1p(x), 1.0 / (1.0 + x))  \
  X(OP_EXPM1, expm1, stan::math::expm1(x), std::exp(x))  \
  X(OP_PHI, Phi, stan::math::Phi(x), stan::math::INV_SQRT_TWO_PI * std::exp(-0.5 * x * x))  \
  X(OP_INV_PHI, inv_Phi, stan::math::inv_Phi(x), 1.0 / (stan::math::INV_SQRT_TWO_PI * std::exp(-0.5 * stan::math::inv_Phi(x) * stan::math::inv_Phi(x))))  \
  X(OP_ERF, erf, std::erf(x), stan::math::TWO_OVER_SQRT_PI * std::exp(-x * x))  \
  X(OP_ERFC, erfc, std::erfc(x), -stan::math::TWO_OVER_SQRT_PI * std::exp(-x * x))  \
  X(OP_INV, inv, 1.0 / x, -1.0 / (x * x))  \
  X(OP_INV_SQRT, inv_sqrt, stan::math::inv_sqrt(x), -0.5 / (x * std::sqrt(x)))  \
  X(OP_INV_SQUARE, inv_square, 1.0 / (x * x), -2.0 / (x * x * x))  \
  X(OP_LOG1M_EXP, log1m_exp, stan::math::log1m_exp(x), -std::exp(x) / (1.0 - std::exp(x)))  \
  X(OP_LOG1P_EXP, log1p_exp, stan::math::log1p_exp(x), stan::math::inv_logit(x))  \
  X(OP_INV_CLOGLOG, inv_cloglog, stan::math::inv_cloglog(x), std::exp(x - std::exp(x)))  \
  X(OP_SIN, sin, std::sin(x), std::cos(x))  \
  X(OP_COS, cos, std::cos(x), -std::sin(x))  \
  X(OP_TAN, tan, std::tan(x), 1.0 / (std::cos(x) * std::cos(x)))  \
  X(OP_ASIN, asin, std::asin(x), 1.0 / std::sqrt(1.0 - x * x))  \
  X(OP_ACOS, acos, std::acos(x), -1.0 / std::sqrt(1.0 - x * x))  \
  X(OP_ATAN, atan, std::atan(x), 1.0 / (1.0 + x * x))  \
  X(OP_SINH, sinh, std::sinh(x), std::cosh(x))  \
  X(OP_COSH, cosh, std::cosh(x), std::sinh(x))  \
  X(OP_ASINH, asinh, std::asinh(x), 1.0 / std::sqrt(x * x + 1.0))  \
  X(OP_ACOSH, acosh, std::acosh(x), 1.0 / std::sqrt(x * x - 1.0))  \
  X(OP_ATANH, atanh, std::atanh(x), 1.0 / (1.0 - x * x))  \
  X(OP_CBRT, cbrt, std::cbrt(x), 1.0 / (3.0 * std::cbrt(x) * std::cbrt(x)))  \
  X(OP_EXP2, exp2, std::exp2(x), stan::math::LOG_TWO * std::exp2(x))  \
  X(OP_LOG2, log2, stan::math::log2(x), 1.0 / (x * stan::math::LOG_TWO))  \
  X(OP_LOG10, log10, std::log10(x), 1.0 / (x * stan::math::LOG_TEN))  \
  X(OP_ABS, abs, std::fabs(x), x < 0 ? -1.0 : 1.0)  \
  X(OP_FLOOR, floor, std::floor(x), 0.0)  \
  X(OP_CEIL, ceil, std::ceil(x), 0.0)  \
  X(OP_ROUND, round, std::round(x), 0.0)  \
  X(OP_TRUNC, trunc, std::trunc(x), 0.0)  \
  X(OP_STEP, step, x < 0 ? 0.0 : 1.0, 0.0)

enum Opcode : uint16_t {
  OP_NONE_ = 0,
#define STANLI_OPCODE_ENUM(name) name,
  STANLI_OPCODE_LIST(STANLI_OPCODE_ENUM)
#undef STANLI_OPCODE_ENUM
#define STANLI_DENSITY_ENUM(code, fn, n, m) code,
  STANLI_SCALAR_DENSITY_LIST(STANLI_DENSITY_ENUM)
#undef STANLI_DENSITY_ENUM
#define STANLI_UNARY_ENUM(code, fn, v, d) code,
  STANLI_SCALAR_UNARY_LIST(STANLI_UNARY_ENUM)
#undef STANLI_UNARY_ENUM
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
