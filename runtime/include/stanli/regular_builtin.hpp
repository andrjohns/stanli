// Shared classification for ordinary scalar functions.  The opcode lists in
// optable.hpp are the source of truth; backends decide only how to materialize
// the selected operation.
#ifndef STANLI_REGULAR_BUILTIN_HPP
#define STANLI_REGULAR_BUILTIN_HPP

#include <stanli/optable.hpp>

#include <cstddef>
#include <optional>
#include <string_view>

namespace stanli {

enum class RegularKind { Binary, BinaryIntFirst, BinaryIntSecond, Unary };

struct RegularSpec {
  RegularKind kind;
  uint16_t opcode;
};

inline std::optional<RegularSpec> resolve_regular_builtin(std::string_view name,
                                                          size_t arity) {
  if (arity == 2) {
    if (name == "log_sum_exp") return RegularSpec{RegularKind::Binary, OP_LSE2};
    if (name == "log_diff_exp")
      return RegularSpec{RegularKind::Binary, OP_LOG_DIFF_EXP};

    struct Named {
      std::string_view name;
      RegularSpec spec;
    };
    static constexpr Named kBinary[] = {
        {"Plus__", {RegularKind::Binary, OP_ADD}},
        {"Minus__", {RegularKind::Binary, OP_SUB}},
        {"Divide__", {RegularKind::Binary, OP_DIV}},
        {"EltTimes__", {RegularKind::Binary, OP_MUL}},
        {"EltDivide__", {RegularKind::Binary, OP_DIV}},
        {"Pow__", {RegularKind::Binary, OP_POW}},
        {"EltPow__", {RegularKind::Binary, OP_POW}},
        {"pow", {RegularKind::Binary, OP_POW}},
        {"add", {RegularKind::Binary, OP_ADD}},
        {"subtract", {RegularKind::Binary, OP_SUB}},
        {"divide", {RegularKind::Binary, OP_DIV}},
        {"elt_multiply", {RegularKind::Binary, OP_MUL}},
        {"elt_divide", {RegularKind::Binary, OP_DIV}},
#define STANLI_REGULAR_BINARY(code, fn_name, fn) \
  {#fn_name, {RegularKind::Binary, code}},
        STANLI_SCALAR_BINARY_LIST(STANLI_REGULAR_BINARY)
#undef STANLI_REGULAR_BINARY
            {"multiply_log", {RegularKind::Binary, OP_LMULTIPLY}},
#define STANLI_REGULAR_INT_FIRST(code, fn_name, fn) \
  {#fn_name, {RegularKind::BinaryIntFirst, code}},
        STANLI_SCALAR_BINARY_INT_FIRST_LIST(STANLI_REGULAR_INT_FIRST)
#undef STANLI_REGULAR_INT_FIRST
#define STANLI_REGULAR_INT_SECOND(code, fn_name, fn) \
  {#fn_name, {RegularKind::BinaryIntSecond, code}},
            STANLI_SCALAR_BINARY_INT_SECOND_LIST(STANLI_REGULAR_INT_SECOND)
#undef STANLI_REGULAR_INT_SECOND
    };
    for (const Named& candidate : kBinary)
      if (candidate.name == name) return candidate.spec;
    return std::nullopt;
  }

  if (arity == 1) {
    struct Named {
      std::string_view name;
      RegularSpec spec;
    };
    static constexpr Named kUnary[] = {
#define STANLI_REGULAR_UNARY(code, fn_name, value, delta, topology) \
  {#fn_name, {RegularKind::Unary, code}},
        STANLI_SCALAR_UNARY_LIST(STANLI_REGULAR_UNARY)
#undef STANLI_REGULAR_UNARY
            {"PMinus__", {RegularKind::Unary, OP_NEG}},
        {"minus", {RegularKind::Unary, OP_NEG}},
        {"std_normal_qf", {RegularKind::Unary, OP_INV_PHI}},
        {"trigamma", {RegularKind::Unary, OP_TRIGAMMA}},
        {"exp", {RegularKind::Unary, OP_EXPV}},
        {"log", {RegularKind::Unary, OP_LOGV}},
        {"inv_logit", {RegularKind::Unary, OP_INV_LOGIT}},
        {"logit", {RegularKind::Unary, OP_LOGIT}},
        {"sqrt", {RegularKind::Unary, OP_SQRT}},
        {"square", {RegularKind::Unary, OP_SQUARE}},
        {"log1m", {RegularKind::Unary, OP_LOG1M}},
        {"softmax", {RegularKind::Unary, OP_SOFTMAX}},
        {"tanh", {RegularKind::Unary, OP_TANHV}},
        {"cumulative_sum", {RegularKind::Unary, OP_CUMSUM}},
        {"log_softmax", {RegularKind::Unary, OP_LOG_SOFTMAX}},
    };
    for (const Named& candidate : kUnary)
      if (candidate.name == name) return candidate.spec;
  }
  return std::nullopt;
}

}  // namespace stanli

#endif
