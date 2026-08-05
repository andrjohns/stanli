// MIR reader over the transformed-MIR sexp of eight schools.
#include <stanrt/mir.hpp>
#include <stanrt/sexp.hpp>

#include <cstdio>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>

static int failures = 0;
static void check(bool ok, const std::string& what) {
  if (!ok) {
    ++failures;
    std::printf("FAIL %s\n", what.c_str());
  }
}

static std::string slurp(const char* path) {
  std::ifstream f(path);
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

int main(int argc, char** argv) {
  using namespace stanrt;
  const char* fix = argc > 1 ? argv[1] : "tests/fixtures/es.tmir.sexp";
  const std::string text = slurp(fix);
  if (text.empty()) {
    std::printf("FAIL fixture missing: %s (run from repo root)\n", fix);
    return 1;
  }
  mir::Program p = mir::read_program(sexp::parse(text));

  // data block
  check(p.input_vars.size() == 3, "3 input vars");
  check(p.input_vars[0].first == "J" && p.input_vars[0].second.base == "SInt",
        "J is SInt");
  check(p.input_vars[1].first == "y" &&
            p.input_vars[1].second.base == "SVector" &&
            p.input_vars[1].second.dims.size() == 1 &&
            p.input_vars[1].second.dims[0].kind == mir::Expr::Var &&
            p.input_vars[1].second.dims[0].name == "J",
        "y is SVector[J]");

  // log_prob: find param decls
  int n_read = 0, n_target = 0;
  const mir::Stmt* tau_decl = nullptr;
  const mir::Stmt* theta_assign = nullptr;
  std::function<void(const mir::Stmt&)> walk = [&](const mir::Stmt& s) {
    if (s.kind == mir::Stmt::Decl && s.read_transform) {
      ++n_read;
      if (s.decl_id == "tau") tau_decl = &s;
    }
    if (s.kind == mir::Stmt::Assignment && s.lhs == "theta") theta_assign = &s;
    if (s.kind == mir::Stmt::TargetPE) ++n_target;
    for (const auto& k : s.body) walk(k);
  };
  for (const auto& s : p.log_prob) walk(s);

  check(n_read == 3, "3 parameter reads");
  check(tau_decl != nullptr, "tau decl found");
  if (tau_decl) {
    check(tau_decl->read_transform->kind == mir::Transform::Lower,
          "tau lower transform");
    check(tau_decl->read_transform->args.size() == 1 &&
              tau_decl->read_transform->args[0].kind == mir::Expr::LitInt &&
              tau_decl->read_transform->args[0].lit_i == 0,
          "tau lower bound 0");
  }
  check(theta_assign != nullptr, "theta assignment found");
  if (theta_assign) {
    const mir::Expr& r = theta_assign->rhs;
    check(r.kind == mir::Expr::FunApp && r.name == "Plus__", "theta rhs plus");
    check(r.args.size() == 2 && r.args[1].kind == mir::Expr::FunApp &&
              r.args[1].name == "Times__",
          "theta rhs times");
    check(r.args[0].kind == mir::Expr::Var && r.args[0].name == "mu" &&
              !r.args[0].data_only,
          "mu var autodiff");
  }
  check(n_target == 4, "4 TargetPE statements");

  // propto flags: all four tildes emit FnLpdf true
  int n_propto = 0;
  std::function<void(const mir::Expr&)> ewalk = [&](const mir::Expr& e) {
    if (e.kind == mir::Expr::FunApp && e.fn_propto) ++n_propto;
    for (const auto& a : e.args) ewalk(a);
  };
  std::function<void(const mir::Stmt&)> swalk = [&](const mir::Stmt& s) {
    if (s.kind == mir::Stmt::TargetPE) ewalk(s.target);
    for (const auto& k : s.body) swalk(k);
  };
  for (const auto& s : p.log_prob) swalk(s);
  check(n_propto == 4, "4 propto lpdf calls");

  if (failures == 0) std::printf("test_mir OK\n");
  return failures == 0 ? 0 : 1;
}
