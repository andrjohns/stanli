// Constraint transform ops: constrained value, log-jacobian, and gradient
// (through both the constrained value and the jacobian) vs stan-math's
// *_constrain var path.
#include <stanli/graph.hpp>
#include <stanli/optable.hpp>

#include <stan/math.hpp>
#include <cstdio>
#include <string>

static int failures = 0;
static void expect_eq(const std::string& what, double got, double want) {
  if (got != want) {
    ++failures;
    std::printf("FAIL %-20s got %.17g want %.17g\n", what.c_str(), got, want);
  }
}

// Graph: lp = sum(constrained) + jac  (sum via dot with ones through
// NORMAL-free ops: use OP_ADD_N over a 1-vector? constrained is a vector;
// use OP_DOT against a ones data vector once Task 4 lands. Here: scalar
// case uses ADD_N directly; vector case multiplies into a normal_lpdf-free
// path via BCAST_FMA trick is overkill, so vector case feeds
// OP_CONSTRAIN_* out into OP_SUM_VEC. OP_SUM_VEC arrives with Task 4; to
// keep Task 3 self-contained it is declared there but implemented here.)
static void run_case(const std::string& tag, uint16_t opcode, int n,
                     const double* x0, double lb, double ub) {
  using namespace stanli;
  using stan::math::var;

  Graph g;
  const int x = g.add_slot(n, true);
  const int b1 = g.add_slot(1, false);
  const int b2 = g.add_slot(1, false);
  const int con = g.add_slot(n, false);
  const int jac = g.add_slot(1, false);
  const int s = g.add_slot(1, false);
  const int lp = g.add_slot(1, false);
  {
    Op op;
    op.opcode = opcode;
    op.out = con;
    op.out2 = jac;
    op.n_in = 0;
    op.in[op.n_in++] = x;
    op.in[op.n_in++] = b1;
    if (opcode == OP_CONSTRAIN_LU) op.in[op.n_in++] = b2;
    g.ops.push_back(op);
  }
  g.add_op(OP_SUM_VEC, {con}, s);
  g.add_op(OP_ADD_N, {s, jac}, lp);
  g.result_slot = lp;

  Executor ex(std::move(g));
  for (int i = 0; i < n; ++i) ex.param_ptr(x)[i] = x0[i];
  ex.value_ptr(b1)[0] = lb;
  ex.value_ptr(b2)[0] = ub;
  double grad[8];
  const double got = ex.gradient(grad);

  // Var reference: lp_ref = sum(constrain(x)) + jac, via *_constrain<true>.
  Eigen::Matrix<var, -1, 1> vx(n);
  for (int i = 0; i < n; ++i) vx(i) = x0[i];
  var vjac = 0.0;
  Eigen::Matrix<var, -1, 1> vcon;
  if (opcode == OP_CONSTRAIN_LOWER) {
    vcon = stan::math::lb_constrain<true>(vx, lb, vjac);
  } else if (opcode == OP_CONSTRAIN_UPPER) {
    vcon = stan::math::ub_constrain<true>(vx, lb, vjac);
  } else {
    vcon = stan::math::lub_constrain<true>(vx, lb, ub, vjac);
  }
  var vlp = stan::math::sum(vcon) + vjac;
  vlp.grad();

  expect_eq(tag + " lp", got, vlp.val());
  for (int i = 0; i < n; ++i)
    expect_eq(tag + " g" + std::to_string(i), grad[i], vx(i).adj());
  stan::math::recover_memory();
}

int main() {
  const double xs[3] = {0.3, -1.2, 2.0};
  const double x1[1] = {0.7};
  run_case("lower vec", stanli::OP_CONSTRAIN_LOWER, 3, xs, 0.0, 0.0);
  run_case("lower scalar", stanli::OP_CONSTRAIN_LOWER, 1, x1, 2.5, 0.0);
  run_case("upper vec", stanli::OP_CONSTRAIN_UPPER, 3, xs, 1.5, 0.0);
  run_case("lu vec", stanli::OP_CONSTRAIN_LU, 3, xs, -1.0, 2.0);
  run_case("lu scalar", stanli::OP_CONSTRAIN_LU, 1, x1, 0.0, 1.0);

  if (failures == 0) std::printf("test_transforms OK\n");
  return failures == 0 ? 0 : 1;
}
