// Decode-only microbenchmark for comparing MIR wire formats without requiring
// model data or paying graph-lowering time.
#include <stanli/mir_decode.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string slurp(const char* path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) return {};
  std::ostringstream contents;
  contents << input.rdbuf();
  return contents.str();
}

size_t program_size(const stanli::mir::Program& program) {
  return program.input_vars.size() + program.prepare_data.size() +
         program.log_prob.size() + program.generate_quantities.size() +
         program.fun_defs.size() + program.output_vars.size();
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2 || argc > 3) {
    std::fprintf(stderr, "usage: bench_mir_decode MIR [REPETITIONS]\n");
    return 2;
  }
  const int repetitions = argc == 3 ? std::atoi(argv[2]) : 51;
  if (repetitions <= 0) {
    std::fprintf(stderr, "bench_mir_decode: repetitions must be positive\n");
    return 2;
  }
  const std::string wire = slurp(argv[1]);
  if (wire.empty()) {
    std::fprintf(stderr, "bench_mir_decode: could not read %s\n", argv[1]);
    return 1;
  }

  try {
    volatile size_t checksum = program_size(stanli::decode_program(wire));
    std::vector<int64_t> samples;
    samples.reserve(static_cast<size_t>(repetitions));
    for (int i = 0; i < repetitions; ++i) {
      const auto start = std::chrono::steady_clock::now();
      stanli::mir::Program program = stanli::decode_program(wire);
      const auto stop = std::chrono::steady_clock::now();
      checksum += program_size(program);
      samples.push_back(
          std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start)
              .count());
    }
    std::sort(samples.begin(), samples.end());
    const size_t median = samples.size() / 2;
    const size_t p95 = (95 * samples.size() + 99) / 100 - 1;
    std::printf(
        "bytes=%zu repetitions=%d min_ns=%lld median_ns=%lld p95_ns=%lld "
        "max_ns=%lld checksum=%zu\n",
        wire.size(), repetitions, static_cast<long long>(samples.front()),
        static_cast<long long>(samples[median]),
        static_cast<long long>(samples[p95]),
        static_cast<long long>(samples.back()), checksum);
  } catch (const std::exception& error) {
    std::fprintf(stderr, "bench_mir_decode: %s\n", error.what());
    return 1;
  }
  return 0;
}
