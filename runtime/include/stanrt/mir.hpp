// C++ representation of the slice of stanc3's transformed MIR that the graph
// compiler consumes. Anything outside the slice is preserved as raw sexp text
// in `raw` and surfaces as a clear compile error, never a miscompile.
#ifndef STANRT_MIR_HPP
#define STANRT_MIR_HPP

#include <stanrt/sexp.hpp>

#include <optional>
#include <string>
#include <vector>

namespace stanrt {
namespace mir {

struct Expr {
  enum Kind { Var, LitInt, LitReal, LitStr, FunApp, Promotion, Indexed,
              Unsupported } kind = Unsupported;
  std::string name;  // Var name or FunApp function name
  enum class Lib { StanLib, Internal, UserDefined } fn_lib = Lib::StanLib;
  bool fn_propto = false;  // (FnLpdf true) / (FnLpmf true)
  long lit_i = 0;
  double lit = 0;
  std::string lit_s;
  std::vector<Expr> args;  // FunApp args; Promotion inner; Indexed base+idx
  std::string type_;       // UInt UReal UVector URowVector UMatrix ...
  bool data_only = false;  // adlevel DataOnly
  std::string raw;         // Unsupported diagnostics
};

struct Transform {
  enum Kind { Identity, Lower, Upper, LowerUpper, Offset, Multiplier,
              OffsetMultiplier, Simplex, Ordered, PositiveOrdered,
              Unsupported } kind = Identity;
  std::vector<Expr> args;
  std::string raw;
};

struct SizedType {
  std::string base;        // SInt SReal SVector SRowVector SMatrix SArray ...
  std::vector<Expr> dims;  // outer-to-inner for SArray chains
  std::string raw;
};

struct Stmt {
  enum Kind { Decl, Assignment, TargetPE, Block, SList, For, IfElse, NRFunApp,
              Skip, Unsupported } kind = Unsupported;
  // Decl
  std::string decl_id;
  SizedType decl_type;
  bool has_init = false;
  Expr init;
  std::optional<Transform> read_transform;  // set iff init is FnReadParam
  std::vector<Expr> read_dims;              // FnReadParam dims
  // Assignment
  std::string lhs;
  std::vector<Expr> lhs_idx;
  Expr rhs;
  // TargetPE
  Expr target;
  // NRFunApp
  std::string fn_name;
  std::vector<Expr> fn_args;
  // For
  std::string loopvar;
  Expr lower, upper;
  // IfElse
  Expr cond;
  // Block / SList / For body / IfElse (then at body[0], else at body[1] if
  // present, each wrapped as its own Stmt)
  std::vector<Stmt> body;
  std::string raw;
};

struct Program {
  std::vector<std::pair<std::string, SizedType>> input_vars;
  std::vector<Stmt> prepare_data;
  std::vector<Stmt> log_prob;
};

Program read_program(const sexp::Node& root);

}  // namespace mir
}  // namespace stanrt

#endif
