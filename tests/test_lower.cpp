// The graph compiler on eight schools: MIR text + data -> graph whose
// log_prob gradient matches a var reference that mirrors the lowering's
// evaluation order. Plus the unsupported-construct error path.
#include <stanrt/compile.hpp>
#include <stanrt/graph.hpp>

#include <stan/math.hpp>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

static int failures = 0;
static void check(bool ok, const std::string& what) {
  if (!ok) {
    ++failures;
    std::printf("FAIL %s\n", what.c_str());
  }
}
static void expect_eq(const std::string& what, double got, double want) {
  if (got != want) {
    ++failures;
    std::printf("FAIL %-16s got %.17g want %.17g\n", what.c_str(), got, want);
  }
}
static std::string slurp(const std::string& path) {
  std::ifstream f(path);
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

static const double kY[8] = {28, 8, -3, 7, -1, 1, 18, 12};
static const double kSigma[8] = {15, 10, 16, 11, 9, 11, 10, 18};

// Mirrors the lowering: reads (with jacobian) in declaration order, then
// statements in program order, target = add_n(terms..., jacs...).
static void reference(const double* q, double* lp_out, double* grad_out) {
  using stan::math::var;
  const int J = 8;
  var mu = q[0];
  var log_tau = q[1];
  Eigen::Matrix<var, -1, 1> tilde(J);
  for (int i = 0; i < J; ++i) tilde(i) = q[2 + i];

  var jac = 0.0;
  var tau = stan::math::lb_constrain<true>(log_tau, 0.0, jac);

  // theta = mu + tau * tilde, lowered as MUL(s,v) then ADD(s,v).
  Eigen::Matrix<var, -1, 1> theta =
      stan::math::add(mu, stan::math::multiply(tau, tilde));

  Eigen::Matrix<var, -1, 1> y(J), sigma(J), zeros(J);
  for (int i = 0; i < J; ++i) {
    y(i) = kY[i];
    sigma(i) = kSigma[i];
  }
  var t1 = stan::math::normal_lpdf<false>(mu, var(0.0), var(5.0));
  var t2 = stan::math::cauchy_lpdf<false>(tau, var(0.0), var(5.0));
  var t3 = stan::math::normal_lpdf<false>(tilde, var(0.0), var(1.0));
  var t4 = stan::math::normal_lpdf<false>(y, theta, sigma);
  var lp = ((((t1 + t2) + t3) + t4) + jac);
  lp.grad();

  *lp_out = lp.val();
  grad_out[0] = mu.adj();
  grad_out[1] = log_tau.adj();
  for (int i = 0; i < J; ++i) grad_out[2 + i] = tilde(i).adj();
  stan::math::recover_memory();
}

int main() {
  using namespace stanrt;

  DataMap data;
  data.set_int("J", 8);
  data.set_real_array("y", std::vector<double>(kY, kY + 8));
  data.set_real_array("sigma", std::vector<double>(kSigma, kSigma + 8));

  CompiledModel cm = compile_model(slurp("tests/fixtures/es.tmir.sexp"), data);
  check(cm.n_unconstrained == 10, "10 unconstrained params");
  check(cm.param_names.size() == 3 && cm.param_names[0] == "mu" &&
            cm.param_names[1] == "tau" && cm.param_names[2] == "theta_tilde",
        "param names");

  Executor ex(std::move(cm.graph));
  cm.bind(ex);

  const double qs[3][10] = {
      {4.0, 1.0, 0.1, -0.2, 0.3, -0.4, 0.5, -0.6, 0.7, -0.8},
      {0.0, -1.5, 1.2, 0.8, -1.1, 0.05, -0.3, 0.9, -1.4, 0.2},
      {-2.5, 0.3, -0.7, 1.5, 0.6, -0.9, 1.1, 0.4, -0.2, -1.3}};
  for (int c = 0; c < 3; ++c) {
    for (int i = 0; i < 10; ++i) ex.params_data()[i] = qs[c][i];
    double grad[10], lp_ref, grad_ref[10];
    const double lp = ex.gradient(grad);
    reference(qs[c], &lp_ref, grad_ref);
    const std::string tag = "case" + std::to_string(c);
    expect_eq(tag + " lp", lp, lp_ref);
    for (int i = 0; i < 10; ++i)
      expect_eq(tag + " g" + std::to_string(i), grad[i], grad_ref[i]);
  }

  // Unsupported construct: for loop must fail with a clear error.
  bool threw = false;
  try {
    compile_model(slurp("tests/fixtures/loopy.tmir.sexp"), [] {
      DataMap d;
      d.set_int("N", 3);
      d.set_real_array("y", {1.0, 2.0, 3.0});
      return d;
    }());
  } catch (const CompileError& e) {
    threw = std::string(e.what()).find("For") != std::string::npos;
  }
  check(threw, "for loop rejected with construct name");

  if (failures == 0) std::printf("test_lower OK\n");
  return failures == 0 ? 0 : 1;
}
