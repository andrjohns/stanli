// reduce_sum's serial lowering.
//
// Stan Math without STAN_THREADS makes exactly one call over the whole slice
// and returns zero for an empty one (prim/functor/reduce_sum.hpp), so stanli
// lowers reduce_sum to that same single call. That is not an approximation to
// be reconciled later: it agrees with default CmdStan term for term. This
// test pins the three things a successful compile would not catch.
//
// 1. The rewritten call is BITWISE the same terms written without
//    reduce_sum. A tolerance here would hide a real change of partition.
// 2. propto rides on the functor's spelling. `partial_sum_lupdf` and
//    `partial_sum_lpdf` name one definition, and only the `_lupdf` form may
//    drop normal's normalizing constant -- the reference for that is the
//    generated rsfunctor's propto__ argument in CmdStan. Getting it
//    backwards still compiles, still samples, and is silently the wrong
//    density. Check (1) covers this because reduce_sum_equiv.stan spells
//    each term's normalization out, so the two models can only agree if
//    both spellings lowered the way CmdStan reads them.
// 3. An empty slice never lowers its callee. The fixture's callee rejects,
//    so a lowering that reached it leaves an OP_REJECT in the graph.
#include <stanli/compile.hpp>
#include <stanli/optable.hpp>

#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void expect(const std::string& what, bool ok) {
  if (!ok) {
    ++failures;
    std::printf("FAIL %s\n", what.c_str());
  }
}

std::string slurp(const std::string& path) {
  std::ifstream f(path);
  std::ostringstream out;
  out << f.rdbuf();
  return out.str();
}

// The same deterministic walk stanli_check uses, so a failure here can be
// reproduced from the command line against the same fixture.
double eval_point(int64_t i) {
  return 0.1 + 0.05 * (double)(i % 7) - 0.15 * (double)(i % 3);
}

struct Run {
  double lp = 0;
  std::vector<double> grad;
};

Run evaluate(stanli::CompiledModel& cm) {
  using namespace stanli;
  Executor ex(std::move(cm.graph));
  cm.bind(ex);
  const int64_t n = ex.n_params();
  Run run;
  run.grad.assign((size_t)n, 0.0);
  for (int64_t i = 0; i < n; ++i) ex.params_data()[i] = eval_point(i);
  run.lp = ex.gradient(run.grad.data());
  return run;
}

int count_opcode(const stanli::Graph& g, uint16_t opcode) {
  int n = 0;
  for (const stanli::Op& op : g.ops)
    if (op.opcode == opcode) ++n;
  return n;
}

}  // namespace

int main() {
  using namespace stanli;

  DataMap data = DataMap::from_json_file("tests/fixtures/reduce_sum.json");
  CompiledModel cm =
      compile_model(slurp("tests/fixtures/reduce_sum.tmir.sexp"), data);

  // Ask the graph before the executor takes it. The empty slice's callee
  // rejects, so reaching it at all is the bug this catches.
  expect("an empty slice never lowers its partial-sum function",
         count_opcode(cm.graph, OP_REJECT) == 0);

  const Run got = evaluate(cm);

  DataMap equiv_data =
      DataMap::from_json_file("tests/fixtures/reduce_sum_equiv.json");
  CompiledModel equiv_cm = compile_model(
      slurp("tests/fixtures/reduce_sum_equiv.tmir.sexp"), equiv_data);
  const Run want = evaluate(equiv_cm);

  expect("lp is finite", std::isfinite(got.lp));
  expect("reduce_sum lp is bitwise the hand-written lp", got.lp == want.lp);
  expect("same parameter count", got.grad.size() == want.grad.size());
  if (got.grad.size() == want.grad.size())
    for (size_t i = 0; i < got.grad.size(); ++i)
      expect("reduce_sum gradient[" + std::to_string(i) +
                 "] is bitwise the hand-written gradient",
             got.grad[i] == want.grad[i]);

  // The equality above is the propto check only if the two spellings would
  // actually disagree at this point. They differ by normal's normalizing
  // constant over the five observations, at the sigma this point implies,
  // so pin that it is not zero and the check cannot pass vacuously.
  const double log_sqrt_2pi = 0.5 * std::log(8.0 * std::atan(1.0));
  const double sigma = std::exp(eval_point(1));  // parameters: mu, sigma, b
  const double normalizer = 5.0 * (log_sqrt_2pi + std::log(sigma));
  expect("the two propto spellings differ by a nonzero constant",
         std::fabs(normalizer) > 1.0);

  if (failures == 0) std::printf("test_reduce_sum: all checks passed\n");
  return failures == 0 ? 0 : 1;
}
