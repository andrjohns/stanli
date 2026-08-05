// CmdStan-side per-gradient latency: the exact loop stan::model::gradient
// runs per leapfrog (fresh vars + grad + recover_memory per call).
#include <stan/io/json/json_data.hpp>
#include <stan/model/model_base.hpp>
#include <stan/math.hpp>
#include <chrono>
#include <cstdio>
#include <fstream>

stan::model::model_base& new_model(stan::io::var_context&, unsigned int,
                                   std::ostream*);

int main(int argc, char** argv) {
  const int N = argc > 2 ? std::atoi(argv[2]) : 100000;
  std::ifstream f(argv[1]);
  stan::json::json_data data(f);
  stan::model::model_base& model = new_model(data, 1, &std::cerr);
  const int64_t n = model.num_params_r();
  Eigen::VectorXd q(n);
  for (int64_t i = 0; i < n; ++i)
    q(i) = 0.1 + 0.05 * (i % 7) - 0.15 * (i % 3);
  Eigen::VectorXd grad(n);
  double sink = 0;
  auto eval = [&](auto propto_fn) {
    Eigen::Matrix<stan::math::var, -1, 1> qv(n);
    for (int64_t i = 0; i < n; ++i) qv(i) = q(i);
    stan::math::var lp = propto_fn(qv);
    lp.grad();
    for (int64_t i = 0; i < n; ++i) grad(i) = qv(i).adj();
    stan::math::recover_memory();
    return lp.val();
  };
  auto full = [&](auto& qv) { return model.log_prob_jacobian(qv, nullptr); };
  auto propto = [&](auto& qv) {
    return model.log_prob_propto_jacobian(qv, nullptr);
  };
  for (int i = 0; i < 1000; ++i) sink += eval(full);
  auto t0 = std::chrono::steady_clock::now();
  for (int i = 0; i < N; ++i) sink += eval(full);
  auto t1 = std::chrono::steady_clock::now();
  for (int i = 0; i < N; ++i) sink += eval(propto);
  auto t2 = std::chrono::steady_clock::now();
  std::printf("cmdstan gradient full(jac):   %.1f ns/eval\n",
              std::chrono::duration<double, std::nano>(t1 - t0).count() / N);
  std::printf("cmdstan gradient propto(jac): %.1f ns/eval (sink %.3g)\n",
              std::chrono::duration<double, std::nano>(t2 - t1).count() / N,
              sink);
  return 0;
}
