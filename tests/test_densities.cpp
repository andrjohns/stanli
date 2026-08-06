// One-op graphs per density: value and every parameter gradient must match
// an in-process var-path evaluation of the same call, bitwise.
#include <stanli/graph.hpp>
#include <stanli/optable.hpp>

#include <stan/math.hpp>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

static int failures = 0;
static void expect_eq(const std::string& what, double got, double want) {
  if (got != want) {
    ++failures;
    std::printf("FAIL %-32s got %.17g want %.17g\n", what.c_str(), got, want);
  }
}
// For comparisons across different instantiation activity. Kernels bind every
// argument as rvar; a reference with data (double) arguments makes stan-math
// take different to_ref_if caching paths, which reassociates shared
// subexpressions. The recorder itself is exact (see the same-activity bitwise
// checks); this bounds the all-rvar evaluation-order divergence.
static void expect_close(const std::string& what, double got, double want) {
  const double tol = 1e-13 * (std::abs(want) > 1 ? std::abs(want) : 1.0);
  if (std::abs(got - want) > tol) {
    ++failures;
    std::printf("FAIL %-32s got %.17g want %.17g (tol %g)\n", what.c_str(),
                got, want, tol);
  }
}

static std::vector<double> ys{1.3, -0.4, 2.2, 0.1, -1.7};
static std::vector<double> pos{0.9, 1.7, 0.35, 2.4, 1.1};
static std::vector<double> unit{0.2, 0.5, 0.75, 0.9, 0.33};
static const int N = 5;

// Build a one-op graph. shapes[i]: len of arg i; params[i]: is arg i a
// parameter. vals[i]: contents. Returns gradient concatenated in param order
// plus the value.
struct RunResult {
  double value;
  std::vector<double> grad;
};
static RunResult run_graph(uint16_t opcode,
                           const std::vector<std::vector<double>>& vals,
                           const std::vector<bool>& params,
                           std::vector<int> idata = {}) {
  using namespace stanli;
  Graph g;
  std::vector<int> slots;
  int64_t n_par = 0;
  for (size_t i = 0; i < vals.size(); ++i) {
    slots.push_back(g.add_slot((int64_t)vals[i].size(), params[i]));
    if (params[i]) n_par += (int64_t)vals[i].size();
  }
  const int lp = g.add_slot(1, false);
  Op op;  // build via add_op with variable input count
  {
    op.opcode = opcode;
    op.out = lp;
    op.n_in = 0;
    for (int s : slots) op.in[op.n_in++] = s;
    if (!idata.empty()) {
      g.idata_pool.push_back(std::move(idata));
      op.idata = g.idata_pool.back().data();
      op.n_idata = (int64_t)g.idata_pool.back().size();
    }
    g.ops.push_back(op);
  }
  g.result_slot = lp;
  Executor ex(std::move(g));
  for (size_t i = 0; i < vals.size(); ++i) {
    double* p = ex.value_ptr(slots[i]);
    for (size_t j = 0; j < vals[i].size(); ++j) p[j] = vals[i][j];
  }
  RunResult r;
  r.grad.assign(n_par, 0.0);
  r.value = ex.gradient(r.grad.data());
  return r;
}

int main() {
  using namespace stanli;
  using stan::math::var;

  // ---- normal_lpdf(y_pv, mu_ps, sigma_ps): all three parameters ----------
  {
    auto r = run_graph(OP_NORMAL_LPDF, {ys, {0.25}, {1.4}},
                       {true, true, true});
    Eigen::Matrix<var, -1, 1> vy(N);
    for (int i = 0; i < N; ++i) vy(i) = ys[i];
    var vmu = 0.25, vsig = 1.4;
    var lp = stan::math::normal_lpdf<false>(vy, vmu, vsig);
    lp.grad();
    expect_eq("normal ppp value", r.value, lp.val());
    for (int i = 0; i < N; ++i)
      expect_eq("normal ppp dy" + std::to_string(i), r.grad[i], vy(i).adj());
    expect_eq("normal ppp dmu", r.grad[N], vmu.adj());
    expect_eq("normal ppp dsigma", r.grad[N + 1], vsig.adj());
    stan::math::recover_memory();
  }

  // ---- normal_lpdf(y_data, mu_ps, sigma_ps) ------------------------------
  {
    auto r = run_graph(OP_NORMAL_LPDF, {ys, {0.25}, {1.4}},
                       {false, true, true});
    Eigen::Map<Eigen::VectorXd> ymap(ys.data(), N);
    var vmu = 0.25, vsig = 1.4;
    var lp = stan::math::normal_lpdf<false>(ymap, vmu, vsig);
    lp.grad();
    expect_eq("normal dpp value", r.value, lp.val());
    expect_eq("normal dpp dmu", r.grad[0], vmu.adj());
    expect_eq("normal dpp dsigma", r.grad[1], vsig.adj());
    stan::math::recover_memory();
  }

  // ---- normal_lpdf(y_data, mu_pv, sigma_data): vector location -----------
  {
    std::vector<double> mus{0.1, -0.2, 0.3, 0.05, -0.6};
    std::vector<double> sigs{15, 10, 16, 11, 9};
    auto r = run_graph(OP_NORMAL_LPDF, {ys, mus, sigs}, {false, true, false});
    Eigen::Map<Eigen::VectorXd> ymap(ys.data(), N);
    Eigen::Map<Eigen::VectorXd> smap(sigs.data(), N);
    Eigen::Matrix<var, -1, 1> vmu(N);
    for (int i = 0; i < N; ++i) vmu(i) = mus[i];
    var lp = stan::math::normal_lpdf<false>(ymap, vmu, smap);
    lp.grad();
    expect_eq("normal dpd value", r.value, lp.val());
    for (int i = 0; i < N; ++i)
      expect_eq("normal dpd dmu" + std::to_string(i), r.grad[i], vmu(i).adj());
    stan::math::recover_memory();
  }

  // ---- cauchy_lpdf(y_ps, 0_data, 5_data) ---------------------------------
  {
    auto r = run_graph(OP_CAUCHY_LPDF, {{2.3}, {0.0}, {5.0}},
                       {true, false, false});
    var vt = 2.3;
    var lp = stan::math::cauchy_lpdf<false>(vt, 0.0, 5.0);
    lp.grad();
    expect_eq("cauchy value", r.value, lp.val());
    expect_eq("cauchy dy", r.grad[0], vt.adj());
    stan::math::recover_memory();
  }

  // ---- student_t_lpdf(y_data, nu_ps, mu_ps, sigma_ps) --------------------
  {
    auto r = run_graph(OP_STUDENT_T_LPDF, {ys, {4.0}, {0.25}, {1.4}},
                       {false, true, true, true});
    Eigen::Map<Eigen::VectorXd> ymap(ys.data(), N);
    var vnu = 4.0, vmu = 0.25, vsig = 1.4;
    var lp = stan::math::student_t_lpdf<false>(ymap, vnu, vmu, vsig);
    lp.grad();
    expect_eq("student_t value", r.value, lp.val());
    expect_close("student_t dnu", r.grad[0], vnu.adj());
    expect_close("student_t dmu", r.grad[1], vmu.adj());
    expect_close("student_t dsigma", r.grad[2], vsig.adj());
    stan::math::recover_memory();

    // Same-activity reference: y promoted to var as the kernel promotes it
    // to rvar. The recorder mechanism must be bitwise here.
    Eigen::Matrix<var, -1, 1> vy(N);
    for (int i = 0; i < N; ++i) vy(i) = ys[i];
    var wnu = 4.0, wmu = 0.25, wsig = 1.4;
    var lp2 = stan::math::student_t_lpdf<false>(vy, wnu, wmu, wsig);
    lp2.grad();
    expect_eq("student_t allvar value", r.value, lp2.val());
    expect_eq("student_t allvar dnu", r.grad[0], wnu.adj());
    expect_eq("student_t allvar dmu", r.grad[1], wmu.adj());
    expect_eq("student_t allvar dsigma", r.grad[2], wsig.adj());
    stan::math::recover_memory();
  }

  // ---- gamma_lpdf(y_data, alpha_ps, beta_ps) -----------------------------
  {
    auto r = run_graph(OP_GAMMA_LPDF, {pos, {2.5}, {1.3}},
                       {false, true, true});
    Eigen::Map<Eigen::VectorXd> ymap(pos.data(), N);
    var va = 2.5, vb = 1.3;
    var lp = stan::math::gamma_lpdf<false>(ymap, va, vb);
    lp.grad();
    expect_eq("gamma value", r.value, lp.val());
    expect_eq("gamma dalpha", r.grad[0], va.adj());
    expect_eq("gamma dbeta", r.grad[1], vb.adj());
    stan::math::recover_memory();
  }

  // ---- beta_lpdf(y_data, alpha_ps, beta_ps) ------------------------------
  {
    auto r = run_graph(OP_BETA_LPDF, {unit, {2.0}, {3.0}},
                       {false, true, true});
    Eigen::Map<Eigen::VectorXd> ymap(unit.data(), N);
    var va = 2.0, vb = 3.0;
    var lp = stan::math::beta_lpdf<false>(ymap, va, vb);
    lp.grad();
    expect_eq("beta value", r.value, lp.val());
    expect_eq("beta dalpha", r.grad[0], va.adj());
    expect_eq("beta dbeta", r.grad[1], vb.adj());
    stan::math::recover_memory();
  }

  // ---- poisson_log_lpmf(n_idata; alpha_pv) -------------------------------
  {
    std::vector<double> alpha{0.2, 0.4, 0.6, 0.8, 1.0};
    auto r = run_graph(OP_POISSON_LOG_LPMF, {alpha}, {true}, {2, 0, 5, 1, 3});
    std::vector<int> n{2, 0, 5, 1, 3};
    Eigen::Matrix<var, -1, 1> va(N);
    for (int i = 0; i < N; ++i) va(i) = alpha[i];
    var lp = stan::math::poisson_log_lpmf<false>(n, va);
    lp.grad();
    expect_eq("poisson_log value", r.value, lp.val());
    for (int i = 0; i < N; ++i)
      expect_eq("poisson_log da" + std::to_string(i), r.grad[i], va(i).adj());
    stan::math::recover_memory();
  }

  // ---- bernoulli_logit_lpmf(y_idata; alpha_pv) ---------------------------
  {
    std::vector<double> alpha{0.5, -1.2, 0.3, 2.0, -0.7};
    auto r = run_graph(OP_BERNOULLI_LOGIT_LPMF, {alpha}, {true},
                       {1, 0, 0, 1, 1});
    std::vector<int> y{1, 0, 0, 1, 1};
    Eigen::Matrix<var, -1, 1> va(N);
    for (int i = 0; i < N; ++i) va(i) = alpha[i];
    var lp = stan::math::bernoulli_logit_lpmf<false>(y, va);
    lp.grad();
    expect_eq("bernoulli_logit value", r.value, lp.val());
    for (int i = 0; i < N; ++i)
      expect_eq("bernoulli_logit da" + std::to_string(i), r.grad[i],
                va(i).adj());
    stan::math::recover_memory();
  }

  if (failures == 0) std::printf("test_densities OK\n");
  return failures == 0 ? 0 : 1;
}
