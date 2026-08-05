// The graph compiler: transformed-MIR sexp text -> executable graph, sized
// against a concrete dataset. M2 scope: straight-line log_prob (tier-1);
// unsupported constructs raise CompileError naming the construct.
//
// Known M2 simplifications, lifted by the corpus loop:
// - FnCheck data validations are skipped (sizes are still enforced when
//   binding data slots); posteriordb data is assumed valid.
// - transformed data expressions are unsupported.
// - propto: all densities lower to propto=false kernels; lp differs from
//   CmdStan's by a per-model constant, gradients are unaffected.
#ifndef STANRT_COMPILE_HPP
#define STANRT_COMPILE_HPP

#include <stanrt/data.hpp>
#include <stanrt/graph.hpp>

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace stanrt {

struct CompileError : std::runtime_error {
  explicit CompileError(const std::string& what) : std::runtime_error(what) {}
};

struct CompiledModel {
  Graph graph;
  std::vector<std::string> param_names;  // declaration order (flat)
  int64_t n_unconstrained = 0;
  // Slot fills for data + constants, applied after Executor construction.
  std::vector<std::pair<int, std::vector<double>>> fills;

  void bind(Executor& ex) const {
    for (const auto& f : fills) {
      double* p = ex.value_ptr(f.first);
      for (size_t j = 0; j < f.second.size(); ++j) p[j] = f.second[j];
    }
  }
};

CompiledModel compile_model(const std::string& tmir_text, const DataMap& data);

}  // namespace stanrt

#endif
