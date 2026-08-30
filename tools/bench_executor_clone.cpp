// Executor clone metadata and steady-state evaluator. Workloads are
// deliberately model-independent scalar graphs with exact analytical
// value/gradient checks. Run baseline and candidate binaries as fresh processes
// so peak RSS is comparable.
#include <stanli/graph.hpp>
#include <stanli/optable.hpp>

#include <chrono>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

#ifdef __APPLE__
#include <mach/mach.h>
#elif defined(__linux__)
#include <fstream>
#include <unistd.h>
#endif
#ifndef _WIN32
#include <sys/resource.h>
#endif

namespace {

struct Config {
  int ops = 25000;
  int executors = 8;
  int reps = 5;
  int dead_payloads = 0;
  std::string workload = "add";
};

bool parse_positive(const char* text, int& out) {
  errno = 0;
  char* end = nullptr;
  const long long value = std::strtoll(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0' || value <= 0 ||
      value > INT_MAX)
    return false;
  out = static_cast<int>(value);
  return true;
}

bool parse_nonnegative(const char* text, int& out) {
  errno = 0;
  char* end = nullptr;
  const long long value = std::strtoll(text, &end, 10);
  if (errno != 0 || end == text || *end != '\0' || value < 0 || value > INT_MAX)
    return false;
  out = static_cast<int>(value);
  return true;
}

bool parse_args(int argc, char** argv, Config& cfg) {
  for (int i = 1; i < argc; i += 2) {
    if (i + 1 >= argc) return false;
    if (std::strcmp(argv[i], "--ops") == 0) {
      if (!parse_positive(argv[i + 1], cfg.ops)) return false;
    } else if (std::strcmp(argv[i], "--executors") == 0) {
      if (!parse_positive(argv[i + 1], cfg.executors)) return false;
    } else if (std::strcmp(argv[i], "--reps") == 0) {
      if (!parse_positive(argv[i + 1], cfg.reps)) return false;
    } else if (std::strcmp(argv[i], "--dead-payloads") == 0) {
      if (!parse_nonnegative(argv[i + 1], cfg.dead_payloads)) return false;
    } else if (std::strcmp(argv[i], "--workload") == 0) {
      cfg.workload = argv[i + 1];
      if (cfg.workload != "add" && cfg.workload != "index") return false;
    } else {
      return false;
    }
  }
  return cfg.ops <= INT_MAX - 2;
}

const char* rss_method() {
#ifdef __APPLE__
  return "mach_task_info";
#elif defined(__linux__)
  return "proc_statm";
#else
  return "unavailable";
#endif
}

double current_rss_mb() {
#ifdef __APPLE__
  mach_task_basic_info_data_t info;
  mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
  if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                reinterpret_cast<task_info_t>(&info), &count) != KERN_SUCCESS)
    return -1.0;
  return static_cast<double>(info.resident_size) / (1024.0 * 1024.0);
#elif defined(__linux__)
  std::ifstream statm("/proc/self/statm");
  long long total = 0, resident = 0;
  if (!(statm >> total >> resident)) return -1.0;
  const long page_size = sysconf(_SC_PAGESIZE);
  if (page_size <= 0) return -1.0;
  return static_cast<double>(resident) * page_size / (1024.0 * 1024.0);
#else
  return -1.0;
#endif
}

double peak_rss_mb() {
#ifdef _WIN32
  return -1.0;
#else
  struct rusage ru;
  if (getrusage(RUSAGE_SELF, &ru) != 0) return -1.0;
#ifdef __APPLE__
  return static_cast<double>(ru.ru_maxrss) / (1024.0 * 1024.0);
#else
  return static_cast<double>(ru.ru_maxrss) / 1024.0;
#endif
#endif
}

struct Chain {
  stanli::Graph graph;
  int constant = -1;
};

template <typename T, typename = void>
struct has_compact_idata : std::false_type {};

template <typename T>
struct has_compact_idata<
    T, std::void_t<decltype(std::declval<T&>().compact_idata())>>
    : std::true_type {};

template <typename T, typename = void>
struct has_integer_storage : std::false_type {};

template <typename T>
struct has_integer_storage<
    T, std::void_t<decltype(std::declval<const T&>().integer_storage_size()),
                   decltype(std::declval<const T&>().integer_storage_blocks())>>
    : std::true_type {};

std::pair<size_t, size_t> idata_shape(const stanli::Graph& graph) {
  size_t elements = 0;
  for (const auto& payload : graph.idata_pool) elements += payload.size();
  return {graph.idata_pool.size(), elements};
}

template <typename T>
std::pair<size_t, size_t> integer_storage_shape(const T& graph) {
  if constexpr (has_integer_storage<T>::value)
    return {static_cast<size_t>(graph.integer_storage_size()),
            static_cast<size_t>(graph.integer_storage_blocks())};
  size_t elements = 0;
  size_t blocks = 0;
  for (const auto& payload : graph.idata_pool) {
    elements += payload.size();
    if (!payload.empty()) ++blocks;
  }
  return {elements, blocks};
}

Chain make_chain(int n_ops, const std::string& workload, int dead_payloads) {
  using namespace stanli;
  Chain chain;
  Graph& graph = chain.graph;
  graph.ops.reserve(static_cast<size_t>(n_ops));
  graph.slots.reserve(static_cast<size_t>(n_ops) + (workload == "add" ? 2 : 1));
  const int parameter = graph.add_slot(1, true);
  int value = parameter;
  if (workload == "add") {
    chain.constant = graph.add_slot(1, false);
    for (int i = 0; i < n_ops; ++i) {
      const int next = graph.add_slot(1, false);
      graph.add_op(OP_ADD, {value, chain.constant}, next);
      value = next;
    }
  } else {
    // Scalar slots are still indexable; this makes every integer payload live
    // while keeping the chain's analytical derivative exactly one.
    for (int i = 0; i < n_ops; ++i) {
      const int next = graph.add_slot(1, false);
      graph.add_op(OP_INDEX, {value}, next, {0});
      value = next;
    }
  }
  for (int i = 0; i < dead_payloads; ++i)
    graph.idata_pool.push_back({0, 1, 2, 3});
  graph.result_slot = value;
  return chain;
}

template <typename T>
double maybe_compact_idata(T& graph) {
  if constexpr (has_compact_idata<T>::value) {
    const auto start = std::chrono::steady_clock::now();
    graph.compact_idata();
    return std::chrono::duration<double, std::milli>(
               std::chrono::steady_clock::now() - start)
        .count();
  }
  return 0.0;
}

template <typename Clock = std::chrono::steady_clock>
double elapsed_ms(typename Clock::time_point start) {
  return std::chrono::duration<double, std::milli>(Clock::now() - start)
      .count();
}

}  // namespace

int main(int argc, char** argv) {
  using namespace stanli;
  using Clock = std::chrono::steady_clock;

  Config cfg;
  if (!parse_args(argc, argv, cfg)) {
    std::fprintf(stderr,
                 "usage: %s [--ops N] [--executors N] [--reps N] "
                 "[--workload add|index] [--dead-payloads N]\n",
                 argv[0]);
    return 2;
  }
  if (find_kernel(OP_ADD) == nullptr ||
      (cfg.workload == "index" && find_kernel(OP_INDEX) == nullptr))
    return 1;

  const auto graph_start = Clock::now();
  Chain chain = make_chain(cfg.ops, cfg.workload, cfg.dead_payloads);
  const double graph_ms = elapsed_ms(graph_start);
  int64_t slot_elements = 0;
  for (const Slot& slot : chain.graph.slots) slot_elements += slot.len;
  const size_t idata_arrays_before = chain.graph.idata_pool.size();
  size_t idata_elements_before = 0;
  for (const auto& payload : chain.graph.idata_pool)
    idata_elements_before += payload.size();
  const auto integer_before = integer_storage_shape(chain.graph);

  const double finalize_ms = maybe_compact_idata(chain.graph);
  const size_t idata_arrays_after = chain.graph.idata_pool.size();
  size_t idata_elements_after = 0;
  for (const auto& payload : chain.graph.idata_pool)
    idata_elements_after += payload.size();
  const auto integer_after = integer_storage_shape(chain.graph);

  const auto bind_start = Clock::now();
  auto prototype = std::make_unique<Executor>(std::move(chain.graph));
  const double bind_ms = elapsed_ms(bind_start);
  const auto idata_after_bind = idata_shape(prototype->graph());
  const auto integer_after_bind = integer_storage_shape(prototype->graph());
  constexpr double increment = 0x1p-20;
  if (chain.constant >= 0) prototype->value_ptr(chain.constant)[0] = increment;
  const double rss_before_clone_mb = current_rss_mb();

  std::vector<std::unique_ptr<Executor>> owned;
  owned.reserve(static_cast<size_t>(cfg.executors - 1));
  const auto clone_start = Clock::now();
  for (int i = 1; i < cfg.executors; ++i)
    owned.push_back(std::make_unique<Executor>(*prototype));
  const double clone_ms = elapsed_ms(clone_start);
  const double rss_after_clone_mb = current_rss_mb();
  const auto idata_after_clone =
      idata_shape(owned.empty() ? prototype->graph() : owned.back()->graph());
  const auto integer_after_clone = integer_storage_shape(
      owned.empty() ? prototype->graph() : owned.back()->graph());

  std::vector<Executor*> executors;
  executors.reserve(static_cast<size_t>(cfg.executors));
  executors.push_back(prototype.get());
  for (auto& clone : owned) executors.push_back(clone.get());

  constexpr double parameter = 0.25;
  double want_value = parameter;
  if (cfg.workload == "add") {
    for (int i = 0; i < cfg.ops; ++i) want_value += increment;
  }
  double sink = 0.0;
  double gradient = 0.0;
  for (Executor* executor : executors) {
    executor->params_data()[0] = parameter;
    const double value = executor->gradient(&gradient);
    if (value != want_value || gradient != 1.0) {
      std::fprintf(stderr,
                   "clone mismatch: value %.17g want %.17g, grad %.17g\n",
                   value, want_value, gradient);
      return 1;
    }
    sink += value + gradient;
  }

  const auto gradient_start = Clock::now();
  for (int rep = 0; rep < cfg.reps; ++rep) {
    for (Executor* executor : executors) {
      executor->params_data()[0] = parameter;
      const double value = executor->gradient(&gradient);
      sink += value + gradient;
    }
  }
  const double gradient_ns =
      std::chrono::duration<double, std::nano>(Clock::now() - gradient_start)
          .count() /
      (static_cast<double>(cfg.reps) * cfg.executors);

  const size_t op_bytes_per_executor =
      prototype->graph().ops.capacity() * sizeof(Op);
  std::printf(
      "ops=%d ops_capacity=%zu slots=%zu slot_elements=%lld executors=%d "
      "reps=%d workload=%s dead_payloads=%d "
      "idata_arrays_before=%zu idata_elements_before=%zu "
      "idata_arrays_after=%zu idata_elements_after=%zu "
      "idata_arrays_after_bind=%zu idata_elements_after_bind=%zu "
      "idata_arrays_after_clone=%zu idata_elements_after_clone=%zu "
      "integer_storage_before=%zu integer_blocks_before=%zu "
      "integer_storage_after=%zu integer_blocks_after=%zu "
      "integer_storage_after_bind=%zu integer_blocks_after_bind=%zu "
      "integer_storage_after_clone=%zu integer_blocks_after_clone=%zu "
      "sizeof_op=%zu sizeof_slot=%zu sizeof_ctx=%zu "
      "op_bytes_per_executor=%zu graph_ms=%.3f finalize_ms=%.3f "
      "bind_ms=%.3f total_setup_first_ms=%.3f total_setup_ms=%.3f "
      "clone_ms_total=%.3f clone_ms_each=%.3f gradient_ns=%.1f "
      "rss_method=%s rss_before_clone_mb=%.3f rss_after_clone_mb=%.3f "
      "peak_rss_mb=%.3f sink=%.17g\n",
      cfg.ops, prototype->graph().ops.capacity(),
      prototype->graph().slots.size(), static_cast<long long>(slot_elements),
      cfg.executors, cfg.reps, cfg.workload.c_str(), cfg.dead_payloads,
      idata_arrays_before, idata_elements_before, idata_arrays_after,
      idata_elements_after, idata_after_bind.first, idata_after_bind.second,
      idata_after_clone.first, idata_after_clone.second, integer_before.first,
      integer_before.second, integer_after.first, integer_after.second,
      integer_after_bind.first, integer_after_bind.second,
      integer_after_clone.first, integer_after_clone.second, sizeof(Op),
      sizeof(Slot), sizeof(KernelCtx), op_bytes_per_executor, graph_ms,
      finalize_ms, bind_ms, graph_ms + finalize_ms + bind_ms,
      graph_ms + finalize_ms + bind_ms + clone_ms, clone_ms,
      cfg.executors > 1 ? clone_ms / (cfg.executors - 1) : 0.0, gradient_ns,
      rss_method(), rss_before_clone_mb, rss_after_clone_mb, peak_rss_mb(),
      sink);
  return 0;
}
