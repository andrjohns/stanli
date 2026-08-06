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

    // Draws are written through the write_array graph when there is one --
    // that is what supplies transformed parameters and generated quantities,
    // and it fixes the column order to CmdStan's. Without it we can still
    // report the constrained parameters the log_prob graph already computes.
    const bool have_wa = cm.write_array && !cm.write_array->columns.empty();
    if (cm.write_array && !cm.write_array->truncated.empty())
      std::fprintf(stderr, "stanli_run: write_array truncated: %s\n",
                   cm.write_array->truncated.c_str());
    std::unique_ptr<stanli::Executor> wex;
    if (have_wa) {
      wex = std::make_unique<stanli::Executor>(
          std::move(cm.write_array->graph));
      cm.write_array->bind(*wex);
    }
    const auto& cols = have_wa ? cm.write_array->columns : cm.views;
    stanli::Executor& out = have_wa ? *wex : ex;

    std::string hdr;
    for (const auto& n : stanli::CompiledModel::csv_names(cols)) {
      if (!hdr.empty()) hdr += ',';
      hdr += n;
    }
    std::printf("%s\n", hdr.c_str());
    for (const auto& q : draws) {
      for (size_t i = 0; i < q.size(); ++i) out.params_data()[i] = q[i];
      out.run_forward_only();
      bool first = true;
      for (const auto& v : cols) {
        const double* p = out.value_ptr(v.slot);
        for (int64_t i = 0; i < v.len; ++i) {
          std::printf(first ? "%.17g" : ",%.17g", p[i]);
          first = false;
        }
      }
      std::printf("\n");
    }
    // Gradient evaluations = leapfrog steps + init probes. Reported so a
    // sampling-time comparison can be split into "cost per gradient" and
    // "how many gradients the sampler asked for", which are different
    // claims and can move in opposite directions.
    std::fprintf(stderr, "stanli_run: %lld gradient evaluations\n",
                 (long long)ex.n_grad_evals());
  } catch (const std::exception& e) {
    std::fprintf(stderr, "stanli_run: %s\n", e.what());
    return 1;
  }
  return 0;
}
