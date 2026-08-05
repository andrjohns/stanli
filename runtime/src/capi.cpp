#include <stanrt/capi.h>

#include <stanrt/compile.hpp>
#include <stanrt/graph.hpp>
#include <stanrt/nuts.hpp>

#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace {

void put_err(char* err, size_t err_len, const char* what) {
  if (err == nullptr || err_len == 0) return;
  std::strncpy(err, what, err_len - 1);
  err[err_len - 1] = '\0';
}

}  // namespace

struct stanrt_model {
  stanrt::CompiledModel cm;   // graph moved out into ex
  std::unique_ptr<stanrt::Executor> ex;
  std::vector<std::string> flat_names;  // constrained, flattened
  int64_t n_con = 0;
};

extern "C" {

stanrt_model* stanrt_model_new(const char* tmir_sexp, const char* data_json,
                               char* err, size_t err_len) {
  try {
    auto m = std::make_unique<stanrt_model>();
    stanrt::DataMap data = stanrt::DataMap::from_json(data_json);
    m->cm = stanrt::compile_model(tmir_sexp, data);
    m->ex = std::make_unique<stanrt::Executor>(std::move(m->cm.graph));
    m->cm.bind(*m->ex);
    for (const auto& v : m->cm.views) {
      m->n_con += v.len;
      for (int64_t i = 0; i < v.len; ++i)
        m->flat_names.push_back(
            v.len == 1 ? v.name : v.name + "." + std::to_string(i + 1));
    }
    return m.release();
  } catch (const std::exception& e) {
    put_err(err, err_len, e.what());
    return nullptr;
  }
}

void stanrt_model_free(stanrt_model* m) { delete m; }

int64_t stanrt_n_unconstrained(const stanrt_model* m) {
  return m->ex->n_params();
}

int stanrt_grad(stanrt_model* m, const double* q, double* lp, double* grad) {
  const int64_t n = m->ex->n_params();
  std::memcpy(m->ex->params_data(), q, sizeof(double) * n);
  try {
    if (grad == nullptr) {
      *lp = m->ex->forward();
    } else {
      *lp = m->ex->gradient(grad);
    }
    return 0;
  } catch (const std::exception&) {
    *lp = -std::numeric_limits<double>::infinity();
    return 1;
  }
}

int stanrt_sample(stanrt_model* m, uint32_t seed, int warmup, int samples,
                  double delta, double* draws, char* err, size_t err_len) {
  try {
    stanrt::NutsConfig cfg;
    cfg.seed = seed;
    cfg.warmup = warmup;
    cfg.samples = samples;
    cfg.delta = delta;
    auto out = stanrt::run_nuts(*m->ex, cfg);
    const int64_t n = m->ex->n_params();
    for (size_t s = 0; s < out.size(); ++s)
      std::memcpy(draws + s * n, out[s].data(), sizeof(double) * n);
    return 0;
  } catch (const std::exception& e) {
    put_err(err, err_len, e.what());
    return 1;
  }
}

int64_t stanrt_n_constrained(const stanrt_model* m) { return m->n_con; }

const char* stanrt_constrained_name(const stanrt_model* m, int64_t i) {
  if (i < 0 || i >= (int64_t)m->flat_names.size()) return nullptr;
  return m->flat_names[i].c_str();
}

int stanrt_constrain(stanrt_model* m, const double* q, double* out) {
  const int64_t n = m->ex->n_params();
  std::memcpy(m->ex->params_data(), q, sizeof(double) * n);
  try {
    m->ex->run_forward_only();
  } catch (const std::exception&) {
    return 1;
  }
  int64_t k = 0;
  for (const auto& v : m->cm.views) {
    const double* p = m->ex->value_ptr(v.slot);
    for (int64_t i = 0; i < v.len; ++i) out[k++] = p[i];
  }
  return 0;
}

}  // extern "C"
