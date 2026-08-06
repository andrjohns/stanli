// Adapter satisfying the slice of the stan model concept the mcmc samplers
// use. The samplers reach the model through stan::model::gradient, which
// instantiates log_prob<propto, jacobian, var>; we answer that with a single
// precomputed_gradients node wrapping the executor's double gradient, so the
// sampler-side var tape holds exactly one vari per gradient evaluation.
//
// propto and jacobian template flags are ignored in M1: the graph already
// includes its Jacobian terms and all densities are propto=false.
#ifndef STANLI_MODEL_ADAPTER_HPP
#define STANLI_MODEL_ADAPTER_HPP

#include <stanli/graph.hpp>

#include <stan/math.hpp>

#include <limits>
#include <ostream>
#include <vector>

namespace stanli {

class ExecutorModel {
 public:
  explicit ExecutorModel(Executor& ex)
      : ex_(&ex), grad_(static_cast<size_t>(ex.n_params())) {}

  size_t num_params_r() const { return static_cast<size_t>(ex_->n_params()); }

  template <bool propto, bool jacobian, typename T>
  T log_prob(Eigen::Matrix<T, -1, 1>& q, std::ostream* /*msgs*/) const {
    const int64_t n = ex_->n_params();
    if constexpr (std::is_same_v<T, double>) {
      for (int64_t i = 0; i < n; ++i) ex_->params_data()[i] = q(i);
      try {
        return ex_->forward();
      } catch (const std::exception&) {
        return -std::numeric_limits<double>::infinity();
      }
    } else {
      static_assert(std::is_same_v<T, stan::math::var>,
                    "adapter supports double and var");
      for (int64_t i = 0; i < n; ++i) ex_->params_data()[i] = q(i).val();
      double value;
      try {
        value = ex_->gradient(grad_.data());
      } catch (const std::exception&) {
        // Rejected point (domain error in a kernel): -inf with no gradient,
        // which the sampler treats as a divergence.
        return T(-std::numeric_limits<double>::infinity());
      }
      std::vector<stan::math::var> ops(q.data(), q.data() + n);
      return stan::math::precomputed_gradients(value, ops, grad_);
    }
  }

 private:
  Executor* ex_;
  mutable std::vector<double> grad_;
};

}  // namespace stanli

#endif
