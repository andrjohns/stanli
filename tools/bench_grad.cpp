// Per-gradient latency: compile model+data, evaluate log_prob gradient at a
// fixed point N times, report ns/eval.
#include <stanrt/compile.hpp>
#include <stanrt/graph.hpp>

#include <chrono>
#include <cstdio>
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
  stanrt::DataMap data = stanrt::DataMap::from_json(slurp(argv[2]));
  stanrt::CompiledModel cm = stanrt::compile_model(slurp(argv[1]), data);
  stanrt::Executor ex(std::move(cm.graph));
  cm.bind(ex);
  const int64_t n = ex.n_params();
  for (int64_t i = 0; i < n; ++i)
    ex.params_data()[i] = 0.1 + 0.05 * (i % 7) - 0.15 * (i % 3);
  std::vector<double> grad(n);
  double sink = 0;
  // warmup
  for (int i = 0; i < 1000; ++i) sink += ex.gradient(grad.data());
  auto t0 = std::chrono::steady_clock::now();
  for (int i = 0; i < N; ++i) sink += ex.gradient(grad.data());
  auto t1 = std::chrono::steady_clock::now();
  const double ns =
      std::chrono::duration<double, std::nano>(t1 - t0).count() / N;
  // Machine-readable: <ns/eval> <sink> <n_params>, consumed by
  // tools/bench_models.py.
  std::printf("%.1f %.6g %lld\n", ns, sink, (long long)n);
  return 0;
}
