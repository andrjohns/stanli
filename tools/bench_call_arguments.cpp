// Developer benchmark for argument acquisition during lowering and for the
// exact-grouping write_array reductions that use transient scratch.
#include <stanli/compile.hpp>
#include <stanli/graph.hpp>
#include <stanli/wa_interp.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <numeric>
#include <new>
#include <sstream>
#include <string>
#include <vector>

using Clock = std::chrono::steady_clock;

static volatile double sink = 0.0;
static thread_local bool track_allocations = false;
static thread_local size_t tracked_bytes = 0;
static thread_local size_t tracked_calls = 0;

void* operator new(std::size_t size) {
  if (track_allocations) {
    tracked_bytes += size;
    ++tracked_calls;
  }
  if (void* memory = std::malloc(size)) return memory;
  throw std::bad_alloc();
}

void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept {
  std::free(memory);
}

static std::string slurp(const char* path) {
  std::ifstream input(path);
  std::ostringstream text;
  text << input.rdbuf();
  return text.str();
}

template <typename F>
static double elapsed_ns(F&& fn) {
  const auto start = Clock::now();
  fn();
  return std::chrono::duration<double, std::nano>(Clock::now() - start).count();
}

static void report(const char* metric, int64_t n, std::vector<double> samples,
                   const char* path) {
  std::sort(samples.begin(), samples.end());
  const double median = samples[samples.size() / 2];
  std::vector<double> deviations;
  deviations.reserve(samples.size());
  for (double sample : samples) deviations.push_back(std::abs(sample - median));
  std::sort(deviations.begin(), deviations.end());
  const double mad = deviations[deviations.size() / 2];
  std::printf(
      "metric=%s n=%lld path=%s samples=%zu median_ns=%.1f mad_ns=%.1f "
      "min_ns=%.1f max_ns=%.1f\n",
      metric, static_cast<long long>(n), path, samples.size(), median, mad,
      samples.front(), samples.back());
}

static stanli::DataMap prep_data(int64_t n) {
  stanli::DataMap data;
  data.set_int("N", n);
  std::vector<double> values(static_cast<size_t>(n));
  for (int64_t i = 0; i < n; ++i)
    values[static_cast<size_t>(i)] = 0.25 + (i % 31) * 0.001;
  data.set_real_array("x", std::move(values), {n});
  return data;
}

static int bench_prep(const std::string& mir, int64_t n, int samples) {
  const stanli::DataMap data = prep_data(n);
  std::vector<double> times;
  std::vector<double> bytes;
  std::vector<double> calls;
  times.reserve(static_cast<size_t>(samples));
  bytes.reserve(static_cast<size_t>(samples));
  calls.reserve(static_cast<size_t>(samples));
  for (int sample = -2; sample < samples; ++sample) {
    tracked_bytes = 0;
    tracked_calls = 0;
    track_allocations = true;
    double ns = elapsed_ns([&] {
      stanli::CompiledModel model = stanli::compile_model(mir, data);
      sink += static_cast<double>(model.graph.ops.size() + model.fills.size());
    });
    track_allocations = false;
    if (sample >= 0) {
      times.push_back(ns);
      bytes.push_back(static_cast<double>(tracked_bytes));
      calls.push_back(static_cast<double>(tracked_calls));
    }
  }
  report("prepare_udf", n, std::move(times), "compiled");
  report("prepare_udf_allocated_bytes", n, std::move(bytes), "compiled");
  report("prepare_udf_allocation_calls", n, std::move(calls), "compiled");
  return 0;
}

static double compiled_row(stanli::Executor& executor,
                           const stanli::CompiledModel::WriteArray& wa,
                           const std::vector<double>& q) {
  std::memcpy(executor.params_data(), q.data(), q.size() * sizeof(double));
  executor.run_forward_only();
  double value = 0.0;
  for (const auto& column : wa.columns) {
    const double* stored = executor.value_ptr(column.slot);
    for (int64_t i = 0; i < column.len; ++i)
      value += stored[column.storage_index(i)];
  }
  return value;
}

static int bench_reduce(const std::string& mir, int64_t n, int samples,
                        int reps) {
  stanli::DataMap data;
  data.set_int("N", n);
  stanli::CompiledModel model = stanli::compile_model(mir, data);
  if (!model.write_array) {
    std::fprintf(stderr, "benchmark model has no write_array path\n");
    return 1;
  }
  std::vector<double> q(static_cast<size_t>(n));
  for (int64_t i = 0; i < n; ++i)
    q[static_cast<size_t>(i)] = 0.999999 + (i % 29) * 1e-9;

  std::vector<double> times;
  times.reserve(static_cast<size_t>(samples));
  if (!model.write_array->interp && model.write_array->truncated.empty()) {
    stanli::Executor executor(model.write_array->graph);
    model.write_array->bind(executor);
    for (int i = 0; i < 3; ++i)
      sink += compiled_row(executor, *model.write_array, q);
    for (int sample = 0; sample < samples; ++sample) {
      const double ns = elapsed_ns([&] {
        for (int rep = 0; rep < reps; ++rep)
          sink += compiled_row(executor, *model.write_array, q);
      });
      times.push_back(ns / reps);
    }
    report("write_array_reductions", n, std::move(times), "compiled");
    return 0;
  }

  if (!model.write_array->interp) {
    std::fprintf(stderr, "truncated write_array has no interpreter\n");
    return 1;
  }
  stanli::Executor parameters(model.graph);
  model.bind(parameters);
  stanli::WaRng rng(1234);
  const auto row = [&] {
    std::memcpy(parameters.params_data(), q.data(), q.size() * sizeof(double));
    parameters.run_forward_only();
    std::vector<double> values =
        model.write_array->interp->eval(model.constrained_env(parameters), rng);
    sink += std::accumulate(values.begin(), values.end(), 0.0);
  };
  for (int i = 0; i < 3; ++i) row();
  for (int sample = 0; sample < samples; ++sample) {
    const double ns = elapsed_ns([&] {
      for (int rep = 0; rep < reps; ++rep) row();
    });
    times.push_back(ns / reps);
  }
  report("write_array_reductions", n, std::move(times), "interpreter");
  return 0;
}

int main(int argc, char** argv) {
  if (argc != 6) {
    std::fprintf(stderr,
                 "usage: %s prep|reduce model.tmir.sexp N samples reps\n",
                 argv[0]);
    return 2;
  }
  const std::string mode = argv[1];
  const std::string mir = slurp(argv[2]);
  const int64_t n = std::strtoll(argv[3], nullptr, 10);
  const int samples = std::atoi(argv[4]);
  const int reps = std::atoi(argv[5]);
  if (n < 2 || samples < 1 || reps < 1) return 2;
  const int result = mode == "prep"     ? bench_prep(mir, n, samples)
                     : mode == "reduce" ? bench_reduce(mir, n, samples, reps)
                                        : 2;
  std::fprintf(stderr, "sink=%.17g\n", sink);
  return result;
}
