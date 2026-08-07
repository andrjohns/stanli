// Per-gradient latency: compile model+data, evaluate log_prob gradient at a
// fixed point N times, report ns/eval.
#include <stanli/compile.hpp>
#include <stanli/graph.hpp>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

static std::string slurp(const char* p) {
  std::ifstream f(p);
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

int main(int argc, char** argv) {
  if (argc < 4) {
    std::fprintf(stderr, "usage: bench_grad mir.sexp data.json N\n");
    return 2;
  }
  const int N = std::atoi(argv[3]);
  stanli::DataMap data = stanli::DataMap::from_json(slurp(argv[2]));
  stanli::CompiledModel cm = stanli::compile_model(slurp(argv[1]), data);
  stanli::Executor ex(std::move(cm.graph));
  cm.bind(ex);
  // STANLI_PROFILE=1: per-opcode time/count/element accounting on stderr.
  // Enabled for the measured loop only, so warmup does not pollute it.
  const char* prof_env = std::getenv("STANLI_PROFILE");
  const bool profile = prof_env && prof_env[0] != '0';
  const int64_t n = ex.n_params();
  for (int64_t i = 0; i < n; ++i)
    ex.params_data()[i] = 0.1 + 0.05 * (i % 7) - 0.15 * (i % 3);
  std::vector<double> grad(n);
  double sink = 0;
  // Warm up by time, not by count: 1000 evaluations is nothing on a scalar
  // model and 90 seconds on an ODE one.
  {
    auto w0 = std::chrono::steady_clock::now();
    for (int i = 0; i < 1000; ++i) {
      sink += ex.gradient(grad.data());
      if (std::chrono::steady_clock::now() - w0 >
          std::chrono::milliseconds(200))
        break;
    }
  }
  if (profile) ex.set_profile(true);
  auto t0 = std::chrono::steady_clock::now();
  for (int i = 0; i < N; ++i) sink += ex.gradient(grad.data());
  auto t1 = std::chrono::steady_clock::now();
  if (profile) std::fprintf(stderr, "%s", ex.profile_report().c_str());
  const double ns =
      std::chrono::duration<double, std::nano>(t1 - t0).count() / N;
  // Forward-only, for splitting a cost between the two sweeps.
  auto t2 = std::chrono::steady_clock::now();
  for (int i = 0; i < N; ++i) ex.run_forward_only();
  auto t3 = std::chrono::steady_clock::now();
  const double fwd_ns =
      std::chrono::duration<double, std::nano>(t3 - t2).count() / N;
  // Machine-readable: <ns/grad> <sink> <ns/forward> <n_params>, consumed by
  // tools/bench_models.py (which reads field 0 and the last field).
  std::printf("%.1f %.6g %.1f %lld\n", ns, sink, fwd_ns, (long long)n);
  return 0;
}
