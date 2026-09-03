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
  // Newton's first control is its scaling step, kept under the Powell name.
  double relative_tolerance = 1e-10;
  double function_tolerance = 1e-6;
  int64_t max_num_steps = 1000;
  Solver solver = Powell;

  void select(const mir::AlgebraCall& call) {
    solver = call.method == mir::AlgebraMethod::Newton ? Newton : Powell;
    variadic = !call.legacy;
    relative_tolerance = solver == Newton ? 1e-3 : 1e-10;
    function_tolerance = 1e-6;
    max_num_steps = call.legacy && solver == Powell ? 1000 : 200;
  }

  // Algebra systems have (unknown, parameters, real data, integer data),
  // while RhsProgram's seed convention has an extra leading scalar time.
  // Lowering compiles a synthetic, unused time formal so the mature ODE
  // register machine can be shared without changing either Stan signature.
};

}  // namespace stanli

#endif
