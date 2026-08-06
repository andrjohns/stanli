// Compile-time description of an ODE solve, attached to its op through the
// graph's user-data channel. Owns everything the integrator needs that is
// fixed at lowering time: the right-hand side's MIR, the function table it
// may call into, the solve times, the data arrays, and the tolerances.
#ifndef STANRT_ODE_HPP
#define STANRT_ODE_HPP

#include <stanrt/mir.hpp>

#include <map>
#include <string>
#include <vector>

namespace stanrt {

struct OdeSpec {
  const mir::FunDef* rhs = nullptr;
  const std::map<std::string, const mir::FunDef*>* funs = nullptr;
  double t0 = 0;
  std::vector<double> ts;
  std::vector<double> x_r;
  std::vector<int> x_i;
  double rtol = 1e-6, atol = 1e-6;
  long max_steps = 1000000;
  bool stiff = false;  // bdf when true, rk45 otherwise
};

}  // namespace stanrt

#endif
