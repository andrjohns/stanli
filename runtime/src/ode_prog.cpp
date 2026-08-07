// The ODE entry into the MIR compiler (mir_prog.hpp). All this adds is
// the integrate_ode_* calling convention: the signature fixes the
// argument order and the sizes, so t, y, theta and x_r get their register
// ranges up front and x_i binds as compile-time integers. Everything the
// body can contain is the shared compiler's problem.
#include <stanli/ode_prog.hpp>

#include <stanli/mir_prog.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace stanli {

RhsProgram compile_rhs(const mir::FunDef& f,
                       const std::map<std::string, const mir::FunDef*>& funs,
                       int n_y, int n_theta, int n_x_r,
                       const std::vector<int>& x_i) {
  RhsProgram p;
  if (f.arg_names.size() != 5) {
    p.why = "right-hand side does not take (t, y, theta, x_r, x_i)";
    return p;
  }
  ProgramCompiler c{p, funs};
  try {
    // The integrate_ode_* signature fixes the argument order and the sizes.
    p.t_reg = c.alloc(1);
    p.y0 = c.alloc(n_y);
    p.th0 = c.alloc(n_theta);
    p.xr0 = c.alloc(n_x_r);
    p.n_y = n_y;
    p.n_th = n_theta;
    p.n_xr = n_x_r;
    c.reals[f.arg_names[0]] = Range{p.t_reg, 1};
    c.reals[f.arg_names[1]] = Range{p.y0, n_y};
    c.reals[f.arg_names[2]] = Range{p.th0, n_theta};
    c.reals[f.arg_names[3]] = Range{p.xr0, n_x_r};
    c.ints[f.arg_names[4]] = std::vector<long>(x_i.begin(), x_i.end());

    Range out{0, 0};
    try {
      for (const auto& s : f.body) c.stmt(s);
      c.bail("right-hand side returned no value");
    } catch (ProgramCompiler::Returned& r) {
      out = r.r;
    }
    if (out.len != n_y)
      c.bail("right-hand side returns " + std::to_string(out.len) +
             " values for " + std::to_string(n_y) + " states");
    for (int k = 0; k < out.len; ++k) p.out_regs.push_back(out.reg + k);
    p.ok = true;
  } catch (Bail& b) {
    p.ok = false;
    p.why = b.why;
    p.code.clear();
    p.out_regs.clear();
  }
  return p;
}

}  // namespace stanli
