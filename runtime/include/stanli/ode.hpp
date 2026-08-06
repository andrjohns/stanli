// Compile-time description of an ODE solve, attached to its op through the
// graph's user-data channel. Owns everything the integrator needs that is
// fixed at lowering time: the right-hand side's MIR, the function table it
// may call into, the solve times, the data arrays, and the tolerances.
#ifndef STANLI_ODE_HPP
#define STANLI_ODE_HPP

#include <stanli/mir.hpp>

#include <map>
#include <string>
#include <vector>

namespace stanli {

struct OdeSpec {
  // The spec outlives lowering (the graph does), so it owns copies of the
  // functions it may call rather than pointing into the parsed program.
  std::map<std::string, mir::FunDef> owned;
  std::map<std::string, const mir::FunDef*> funs_map;
  std::string rhs_name;

  void adopt(const std::map<std::string, const mir::FunDef*>& src) {
    for (const auto& [name, def] : src) owned[name] = *def;
    for (const auto& [name, def] : owned) funs_map[name] = &def;
  }
  const mir::FunDef* rhs() const {
    auto it = owned.find(rhs_name);
    return it == owned.end() ? nullptr : &it->second;
  }
  const std::map<std::string, const mir::FunDef*>* funs() const {
    return &funs_map;
  }
  double t0 = 0;
  std::vector<double> ts;
  std::vector<double> x_r;
  std::vector<int> x_i;
  double rtol = 1e-6, atol = 1e-6;
  long max_steps = 1000000;
  bool stiff = false;  // bdf when true, rk45 otherwise
};

}  // namespace stanli

#endif
