// stanli_run: model.stan + data.json -> posterior draws CSV on stdout.
// The full user path: runs the stanc binary for MIR, compiles the graph,
// samples with NUTS, emits constrained parameter draws.
//
// Usage: stanli_run model.stan data.json [--seed N] [--warmup N]
//        [--samples N] [--delta X] [--stanc PATH]
#include <stanli/compile.hpp>
#include <stanli/nuts.hpp>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

static std::string run_stanc(const std::string& stanc,
                             const std::string& model) {
  const std::string cmd = stanc + " --debug-transformed-mir '" + model +
                          "' 2>/dev/null";
  std::unique_ptr<FILE, int (*)(FILE*)> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe) throw std::runtime_error("cannot run stanc: " + cmd);
  std::string out;
  std::array<char, 1 << 16> buf;
  size_t n;
  while ((n = fread(buf.data(), 1, buf.size(), pipe.get())) > 0)
    out.append(buf.data(), n);
  if (out.empty())
    throw std::runtime_error("stanc produced no MIR (compile error?)");
  return out;
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr,
                 "usage: stanli_run model.stan data.json [--seed N] "
                 "[--warmup N] [--samples N] [--delta X] [--stanc PATH]\n");
    return 2;
  }
  std::string model = argv[1], datafile = argv[2];
  std::string stanc = "deps/stanc3/stanc";
  stanli::NutsConfig cfg;
  cfg.seed = 1;
  cfg.warmup = 1000;
  cfg.samples = 1000;
  for (int i = 3; i + 1 < argc; i += 2) {
    const std::string k = argv[i], v = argv[i + 1];
    if (k == "--seed") cfg.seed = (uint32_t)std::stoul(v);
    else if (k == "--warmup") cfg.warmup = std::stoi(v);
    else if (k == "--samples") cfg.samples = std::stoi(v);
    else if (k == "--delta") cfg.delta = std::stod(v);
    else if (k == "--stanc") stanc = v;
  }
  if (const char* env = std::getenv("STANC")) stanc = env;

  try {
    stanli::DataMap data = stanli::DataMap::from_json_file(datafile);
    stanli::CompiledModel cm =
        stanli::compile_model(run_stanc(stanc, model), data);
    stanli::Executor ex(std::move(cm.graph));
    cm.bind(ex);
    auto draws = stanli::run_nuts(ex, cfg);

    // Header: constrained parameter names (flattened).
    std::string hdr;
    for (const auto& v : cm.views) {
      for (int64_t i = 0; i < v.len; ++i) {
        if (!hdr.empty()) hdr += ',';
        hdr += v.len == 1 ? v.name
                          : v.name + "." + std::to_string(i + 1);
      }
    }
    std::printf("%s\n", hdr.c_str());
    for (const auto& q : draws) {
      for (size_t i = 0; i < q.size(); ++i) ex.params_data()[i] = q[i];
      ex.run_forward_only();
      bool first = true;
      for (const auto& v : cm.views) {
        const double* p = ex.value_ptr(v.slot);
        for (int64_t i = 0; i < v.len; ++i) {
          std::printf(first ? "%.17g" : ",%.17g", p[i]);
          first = false;
        }
      }
      std::printf("\n");
    }
  } catch (const std::exception& e) {
    std::fprintf(stderr, "stanli_run: %s\n", e.what());
    return 1;
  }
  return 0;
}
