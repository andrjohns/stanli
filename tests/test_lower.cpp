// The graph compiler on eight schools: MIR text + data -> graph whose
// log_prob gradient matches a var reference that mirrors the lowering's
// evaluation order. Plus the unsupported-construct error path.
#include <stanrt/compile.hpp>
#include <stanrt/graph.hpp>

#include <stan/math.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
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
static int64_t ulp_key(double d) {
  int64_t i;
  std::memcpy(&i, &d, sizeof(i));
  return i < 0 ? std::numeric_limits<int64_t>::min() - i : i;
}
// Project parity budget: up to 2 ULP vs references is acceptable.
static void expect_ulp(const std::string& what, double got, double want) {
  const int64_t d = std::llabs(ulp_key(got) - ulp_key(want));
  if (d > 2) {
    ++failures;
    std::printf("FAIL %-16s got %.17g want %.17g (%lld ulp)\n", what.c_str(),
                got, want, (long long)d);
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

  // ~ statements lower propto=true with activity from MIR adlevels: exactly
  // the instantiations CmdStan's generated C++ uses (data args stay double).
  Eigen::Map<const Eigen::VectorXd> y(kY, J);
  Eigen::Map<const Eigen::VectorXd> sigma(kSigma, J);
  var t1 = stan::math::normal_lpdf<true>(mu, 0.0, 5.0);
  var t2 = stan::math::cauchy_lpdf<true>(tau, 0.0, 5.0);
  var t3 = stan::math::normal_lpdf<true>(tilde, 0.0, 1.0);
  var t4 = stan::math::normal_lpdf<true>(y, theta, sigma);
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

  // For loops unroll: scalar-loop normal model vs per-term var reference.
  {
    DataMap d;
    d.set_int("N", 3);
    d.set_real_array("y", {1.0, 2.0, 3.0});
    CompiledModel lm = compile_model(slurp("tests/fixtures/loopy.tmir.sexp"), d);
    Executor lex(std::move(lm.graph));
    lm.bind(lex);
    lex.params_data()[0] = 0.4;
    double g = 0, lp = lex.gradient(&g);

    using stan::math::var;
    var mu = 0.4;
    const double yv[3] = {1.0, 2.0, 3.0};
    var acc = 0.0;
    for (int n = 0; n < 3; ++n)
      acc = acc + stan::math::normal_lpdf<true>(yv[n], mu, 1.0);
    acc.grad();
    expect_eq("loopy lp", lp, acc.val());
    expect_eq("loopy dmu", g, mu.adj());
    stan::math::recover_memory();
  }

  // Static if inside an unrolled loop, condition indexing data by the loop
  // variable (M0 capture-recapture pattern): the branch is resolved at
  // compile time per iteration.
  {
    DataMap d;
    d.set_int("M", 3);
    d.set_int_array("s", {2, 0, 1});
    CompiledModel lm = compile_model(slurp("tests/fixtures/staticif.tmir.sexp"), d);
    Executor lex(std::move(lm.graph));
    lm.bind(lex);
    lex.params_data()[0] = 0.3;
    double g = 0, lp = lex.gradient(&g);

    using stan::math::var;
    var pu = 0.3;
    // Mirror the lowering: model terms sum first, jacobians append last.
    var lj = 0.0;
    var pc = stan::math::lub_constrain(pu, 0.0, 1.0, lj);
    var acc = 0.0;
    const int sv[3] = {2, 0, 1};
    for (int i = 0; i < 3; ++i) {
      if (sv[i] > 0)
        acc = acc + stan::math::binomial_lpmf<false>(sv[i], 5, pc);
      else
        acc = acc + stan::math::bernoulli_lpmf<false>(0, pc);
    }
    acc = acc + lj;
    acc.grad();
    expect_eq("staticif lp", lp, acc.val());
    expect_eq("staticif dp", g, pu.adj());
    stan::math::recover_memory();
  }

  // Row of a 2-D int data array as a density outcome: y[i] reaches the
  // kernel as a T-length int array (Mb/Mt/irt_2pl pattern).
  {
    DataMap d = DataMap::from_json(
        R"({"M": 2, "T": 3, "y": [[1, 0, 1], [0, 0, 1]]})");
    CompiledModel lm = compile_model(slurp("tests/fixtures/introw.tmir.sexp"), d);
    check(lm.n_unconstrained == 3, "introw 3 unconstrained");
    Executor lex(std::move(lm.graph));
    lm.bind(lex);
    for (int i = 0; i < 3; ++i) lex.params_data()[i] = 0.2 * (i + 1) - 0.3;
    double grad[3] = {0, 0, 0};
    const double lp = lex.gradient(grad);

    using stan::math::var;
    Eigen::Matrix<var, -1, 1> pu(3);
    for (int i = 0; i < 3; ++i) pu(i) = 0.2 * (i + 1) - 0.3;
    var lj = 0.0;
    Eigen::Matrix<var, -1, 1> pc =
        stan::math::lub_constrain(pu, 0.0, 1.0, lj);
    const std::vector<std::vector<int>> yv = {{1, 0, 1}, {0, 0, 1}};
    var acc = 0.0;
    for (int i = 0; i < 2; ++i)
      acc = acc + stan::math::bernoulli_lpmf<false>(yv[i], pc);
    acc = acc + lj;
    acc.grad();
    expect_eq("introw lp", lp, acc.val());
    for (int i = 0; i < 3; ++i)
      expect_eq("introw g" + std::to_string(i), grad[i], pu(i).adj());
    stan::math::recover_memory();
  }

  // Data-only expressions with no native lowering const-fold at compile
  // time: mean/sd bounds, negative_infinity in log_sum_exp, constant lccdf.
  {
    DataMap d;
    d.set_int("N", 2);
    d.set_real_array("y", {1.3, -0.7});
    CompiledModel lm = compile_model(slurp("tests/fixtures/dfold.tmir.sexp"), d);
    Executor lex(std::move(lm.graph));
    lm.bind(lex);
    lex.params_data()[0] = 0.25;
    lex.params_data()[1] = -0.6;
    double grad[2] = {0, 0};
    const double lp = lex.gradient(grad);

    using stan::math::var;
    const double yv[2] = {1.3, -0.7};
    const double m = (yv[0] + yv[1]) / 2.0;
    const double s = std::sqrt(((yv[0] - m) * (yv[0] - m) +
                                (yv[1] - m) * (yv[1] - m)) / 1.0);
    Eigen::Matrix<var, -1, 1> muu(2);
    muu << 0.25, -0.6;
    var lj = 0.0;
    Eigen::Matrix<var, -1, 1> muc =
        stan::math::lub_constrain(muu, m - 3 * s, m + 3 * s, lj);
    var acc = stan::math::normal_lpdf<false>(yv[0], muc(0), 1.0);
    acc = acc + stan::math::log_sum_exp(
                    stan::math::normal_lpdf<false>(yv[1], muc(1), 1.0),
                    var(-std::numeric_limits<double>::infinity()));
    acc = acc + stan::math::student_t_lccdf(0.0, 3.0, 0.0, 10.0);
    acc = acc + lj;
    acc.grad();
    expect_ulp("dfold lp", lp, acc.val());
    for (int i = 0; i < 2; ++i)
      expect_ulp("dfold g" + std::to_string(i), grad[i], muu(i).adj());
    stan::math::recover_memory();
  }

  // Simplex + dirichlet: gradient vs the var path (simplex_constrain and
  // dirichlet_lpdf composed exactly as the lowering emits them).
  {
    DataMap d;
    d.set_int("K", 3);
    CompiledModel sm = compile_model(slurp("tests/fixtures/simp.tmir.sexp"), d);
    check(sm.n_unconstrained == 2, "simplex K-1 unconstrained");
    Executor sex(std::move(sm.graph));
    sm.bind(sex);
    sex.params_data()[0] = 0.3;
    sex.params_data()[1] = -0.8;
    double sg[2], slp = sex.gradient(sg);

    using stan::math::var;
    Eigen::Matrix<var, -1, 1> y(2);
    y(0) = 0.3;
    y(1) = -0.8;
    var jac = 0.0;
    auto theta = stan::math::simplex_constrain(y, jac);
    Eigen::VectorXd alpha(3);
    for (int i = 0; i < 3; ++i) alpha(i) = 2.0;
    var lp = stan::math::dirichlet_lpdf<true>(theta, alpha) + jac;
    lp.grad();
    expect_eq("simplex lp", slp, lp.val());
    expect_eq("simplex g0", sg[0], y(0).adj());
    expect_eq("simplex g1", sg[1], y(1).adj());
    stan::math::recover_memory();
  }

  // Unsupported construct: cholesky_factor_corr must fail clearly.
  bool threw = false;
  try {
    compile_model(slurp("tests/fixtures/chol.tmir.sexp"), [] {
      DataMap d;
      d.set_int("K", 3);
      return d;
    }());
  } catch (const CompileError& e) {
    threw = std::string(e.what()).find("transform") != std::string::npos;
  }
  check(threw, "cholesky_corr rejected with construct name");

  if (failures == 0) std::printf("test_lower OK\n");
  return failures == 0 ? 0 : 1;
}
