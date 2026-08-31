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
#include <stanli/message_sink.hpp>
#include <stanli/optable.hpp>

#include <cmath>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
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

void argument_evaluation() {
  using namespace stanli;
  const std::string mir =
      slurp("tests/fixtures/reduce_sum_arguments.tmir.sexp");
  for (int in_td : {0, 1})
    for (int n : {0, 2})
      for (int grain : {0, 1})
        for (int refuse : {0, 1}) {
          const std::string tag = "arguments td=" + std::to_string(in_td) +
                                  " n=" + std::to_string(n) +
                                  " grain=" + std::to_string(grain) +
                                  " refuse=" + std::to_string(refuse);
          DataMap data =
              DataMap::from_json("{\"N\":" + std::to_string(n) +
                                 ",\"y\":" + (n == 0 ? "[]" : "[0.5,-0.2]") +
                                 ",\"grainsize\":" + std::to_string(grain) +
                                 ",\"refuse\":" + std::to_string(refuse) +
                                 ",\"in_td\":" + std::to_string(in_td) + "}");
          std::vector<std::string> lines;
          std::string error;
          bool evaluating = false;
          bool domain_error = false;
          set_message_sink([&](const char* text, size_t len) {
            lines.emplace_back(text, len);
          });
          try {
            CompiledModel cm = compile_model(mir, data);
            if (!in_td)
              expect(tag + " no effects while lowering", lines.empty());
            Executor ex(std::move(cm.graph));
            cm.bind(ex);
            ex.params_data()[0] = 0.25;
            double grad;
            evaluating = true;
            expect(tag + " finite lp", std::isfinite(ex.gradient(&grad)));
            if (!in_td) {
              const auto first = lines;
              lines.clear();
              ex.gradient(&grad);
              expect(tag + " effects once per evaluation", lines == first);
            }
          } catch (const std::domain_error& e) {
            domain_error = true;
            error = e.what();
          } catch (const std::exception& e) {
            error = e.what();
          }
          set_message_sink(nullptr);
          expect(tag + " rejection", !error.empty() == (grain == 0 || refuse));
          if (!error.empty()) {
            expect(tag + " error: " + error,
                   error.find(refuse ? "shared argument rejected"
                                     : "grainsize") != std::string::npos);
            expect(tag + " correct execution phase", evaluating == !in_td);
            if (!in_td) expect(tag + " runtime domain_error", domain_error);
          }
          // C++ leaves argument order unspecified; require both evaluations
          // exactly once before entering the partial-sum body or validating.
          expect(tag + " grain evaluated once",
                 std::count(lines.begin(), lines.end(),
                            "grain=" + std::to_string(grain)) == 1);
          expect(tag + " shared evaluated once",
                 std::count(lines.begin(), lines.end(), "shared=0.25") == 1);
          const bool body_runs = n > 0 && grain > 0 && !refuse;
          expect(tag + " partial body only when required",
                 lines.size() == (body_runs ? 3u : 2u));
          if (body_runs && !lines.empty())
            expect(tag + " whole-slice bounds",
                   lines.back() == "partial bounds=1:2");
        }
}

void shapes_and_overloads() {
  using namespace stanli;
  CompiledModel cm = compile_model(
      slurp("tests/fixtures/reduce_sum_shapes.tmir.sexp"), DataMap{});
  Executor ex(std::move(cm.graph));
  cm.bind(ex);
  expect("shape fixture parameter count", ex.n_params() == 4);
  if (ex.n_params() != 4) return;
  for (double scale : {-0.5, 0.0, 1.3}) {
    double total = 0;
    for (int i = 0; i < 4; ++i) {
      ex.params_data()[i] = scale * (i + 1);
      if (i < 3) total += ex.params_data()[i];
    }
    const double b = ex.params_data()[3];
    double grad[4];
    const double lp = ex.gradient(grad);
    // The independent polynomial oracle is 3*b*sum(z) + 6*b. The tiny
    // absolute tolerance allows the different addition groupings only.
    expect("nested/overloaded lp", std::abs(lp - b * (3 * total + 6)) < 1e-12);
    for (int i = 0; i < 3; ++i)
      expect("active sliced gradient", std::abs(grad[i] - 3 * b) < 1e-12);
    expect("shared gradient", std::abs(grad[3] - (3 * total + 6)) < 1e-12);
  }
}

}  // namespace

int main() {
  using namespace stanli;

  argument_evaluation();
  shapes_and_overloads();

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
