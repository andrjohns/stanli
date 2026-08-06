// Reference driver: compiled per model against CmdStan's generated .hpp
// (passed via -include). Evaluates log_prob_propto_jacobian (the sampling
// semantics; stanrt lowers ~ statements propto with matched activity) and
// its gradient at the deterministic stanrt_check point.
// Output: OK <lp> <g0> <g1> ...
#include <stan/io/json/json_data.hpp>
#include <stan/model/model_base.hpp>
#include <stan/math.hpp>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <vector>

// Provided by the generated model translation unit.
stan::model::model_base& new_model(stan::io::var_context& data_context,
                                   unsigned int seed,
                                   std::ostream* msg_stream);

// Same points as tools/stanrt_check.cpp; see the note there on why more
// than one exists.
static double eval_point(int64_t i, int variant) {
  switch (variant) {
    case 1:
      return 0.02 * static_cast<double>((i % 5) - 2);
    case 2:
      return 0.0;
    default:
      return 0.1 + 0.05 * static_cast<double>(i % 7) -
             0.15 * static_cast<double>(i % 3);
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: ref_driver data.json [point]\n");
    return 2;
  }
  const int variant = argc > 2 ? std::atoi(argv[2]) : 0;
  std::ifstream f(argv[1]);
  stan::json::json_data data(f);
  stan::model::model_base& model = new_model(data, 1, &std::cerr);

  const int64_t n = static_cast<int64_t>(model.num_params_r());
  Eigen::Matrix<stan::math::var, -1, 1> q(n);
  for (int64_t i = 0; i < n; ++i) q(i) = eval_point(i, variant);

  stan::math::var lp = model.log_prob_propto_jacobian(q, &std::cerr);
  lp.grad();
  std::printf("OK %.17g", lp.val());
  for (int64_t i = 0; i < n; ++i) std::printf(" %.17g", q(i).adj());
  std::printf("\n");
  return 0;
}
