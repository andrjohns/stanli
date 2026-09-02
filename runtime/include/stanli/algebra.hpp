// Compile-time description of a legacy algebra_solver call.  The algebraic
// system remains callable after lowering because the root finder chooses the
// unknown vector values at runtime.  This owns the MIR function table, data,
// tolerances, and (when possible) a compiled register program for the system.
#ifndef STANLI_ALGEBRA_HPP
#define STANLI_ALGEBRA_HPP

#include <stanli/callback.hpp>

#include <cstdint>
#include <vector>

namespace stanli {

struct AlgebraSpec : RetainedCallback {
  enum Solver { Powell, Newton };

  // Compatibility name retained for tooling/tests; storage and lookup live
  // in the shared RetainedCallback base.
  std::string system_name;
  const mir::FunDef* system() const { return callback(system_name); }

  // Variadic solve_* calls pack all active real callback arguments into the
  // kernel's second input and all data reals into x_r. `args` reconstructs
  // the original positional signature for the compiled/interpreted callback.
  bool variadic = false;
  double relative_tolerance = 1e-10;
  double function_tolerance = 1e-6;
  int64_t max_num_steps = 1000;
  Solver solver = Powell;

  // Algebra systems have (unknown, parameters, real data, integer data),
  // while RhsProgram's seed convention has an extra leading scalar time.
  // Lowering compiles a synthetic, unused time formal so the mature ODE
  // register machine can be shared without changing either Stan signature.
};

}  // namespace stanli

#endif
