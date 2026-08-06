// stanrt_check: compile model.stan + data.json and evaluate log_prob +
// gradient at a deterministic unconstrained point. Machine-readable output:
//   OK <lp> <g0> <g1> ...        on success
//   COMPILE_FAIL <first line of error>
//   EVAL_FAIL <what>
// Used by tools/corpus.py to build the coverage scoreboard, and by the
// reference harness to compare against CmdStan at the same point.
#include <stanrt/compile.hpp>
#include <stanrt/graph.hpp>

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

// Deterministic evaluation points, shared with tools/ref_driver.cpp. Some
// models are invalid at a given point (an ODE solution dips below a declared
// lower bound, say) and both engines reject it; the harness then retries the
// next variant so those models still get a real comparison.
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

static std::string run_stanc(const std::string& stanc,
                             const std::string& model) {
  const std::string cmd = stanc + " --debug-transformed-mir '" + model +
                          "' 2>/dev/null";
  std::unique_ptr<FILE, int (*)(FILE*)> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe) throw std::runtime_error("cannot run stanc");
  std::string out;
  std::array<char, 1 << 16> buf;
  size_t n;
  while ((n = fread(buf.data(), 1, buf.size(), pipe.get())) > 0)
    out.append(buf.data(), n);
  return out;
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: stanrt_check model.stan data.json [--stanc PATH] [--point N]\n");
    return 2;
  }
  std::string stanc = "deps/stanc3/stanc";
  int variant = 0;
  for (int i = 3; i + 1 < argc; i += 2) {
    if (std::string(argv[i]) == "--stanc") stanc = argv[i + 1];
    if (std::string(argv[i]) == "--point") variant = std::atoi(argv[i + 1]);
  }
  if (const char* env = std::getenv("STANC")) stanc = env;

  std::string mir;
  try {
    mir = run_stanc(stanc, argv[1]);
    if (mir.empty()) {
      std::printf("COMPILE_FAIL stanc produced no MIR\n");
      return 1;
    }
  } catch (const std::exception& e) {
    std::printf("COMPILE_FAIL stanc: %s\n", e.what());
    return 1;
  }

  stanrt::CompiledModel cm;
  try {
    stanrt::DataMap data = stanrt::DataMap::from_json_file(argv[2]);
    cm = stanrt::compile_model(mir, data);
  } catch (const std::exception& e) {
    std::string what = e.what();
    const size_t nl = what.find('\n');
    if (nl != std::string::npos) what = what.substr(0, nl);
    std::printf("COMPILE_FAIL %s\n", what.c_str());
    return 1;
  }

  try {
    stanrt::Executor ex(std::move(cm.graph));
    cm.bind(ex);
    const int64_t n = ex.n_params();
    for (int64_t i = 0; i < n; ++i)
      ex.params_data()[i] = eval_point(i, variant);
    std::vector<double> grad(n, 0.0);
    const double lp = ex.gradient(grad.data());
    if (!std::isfinite(lp)) {
      std::printf("EVAL_FAIL nonfinite lp\n");
      return 1;
    }
    for (double g : grad)
      if (!std::isfinite(g)) {
        std::printf("EVAL_FAIL nonfinite gradient\n");
        return 1;
      }
    std::printf("OK %.17g", lp);
    for (double g : grad) std::printf(" %.17g", g);
    std::printf("\n");
  } catch (const std::exception& e) {
    std::printf("EVAL_FAIL %s\n", e.what());
    return 1;
  }
  return 0;
}
