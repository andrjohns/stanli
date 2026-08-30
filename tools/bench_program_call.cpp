// Per-site overhead of Program::CALL in its forward and generated-adjoint
// sweeps.  The repeated kernel is deliberately selected by opcode and shape,
// not by any source/model identity.  OP_POW has a cheap scalar body, no
// scratch, and a backward that reads its input and output values, so the
// benchmark exercises the complete CALL packet without letting setup or a
// heavyweight kernel dominate it.
#include <stanli/island.hpp>
#include <stanli/optable.hpp>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace {

constexpr int kSites = 4096;
constexpr int kReps = 2000;

template <typename F>
double time_ns(F&& fn) {
  fn();
  fn();
  const auto start = std::chrono::steady_clock::now();
  for (int i = 0; i < kReps; ++i) fn();
  return std::chrono::duration<double, std::nano>(
             std::chrono::steady_clock::now() - start)
             .count() /
         kReps;
}

stanli::IslandProg make_program() {
  using namespace stanli;
  IslandProg p;
  p.n_regs = 2 + kSites;
  p.ins.push_back(IslandProg::LiveIn{0, 1, 0, 0, true});
  p.ins.push_back(IslandProg::LiveIn{1, 1, 1, 0, true});
  p.calls.reserve(kSites);
  p.code.reserve(kSites);
  p.out_regs.reserve(kSites);
  for (int i = 0; i < kSites; ++i) {
    Program::Call call;
    call.opcode = OP_POW;
    call.n_in = 2;
    call.in[0] = 0;
    call.in[1] = 1;
    call.in_len[0] = 1;
    call.in_len[1] = 1;
    call.out = 2 + i;
    call.out_len = 1;
    if (!bind_call(call)) throw std::runtime_error("CALL bind refused");
    p.calls.push_back(std::move(call));
    p.code.push_back(Program::Instr{Program::CALL, 0, i, 0, 0, 0});
    p.out_regs.push_back(2 + i);
  }
  if (!gen_adjoint(p)) throw std::runtime_error("CALL adjoint refused");
  return p;
}

}  // namespace

int main() {
  using namespace stanli;
  IslandProg p = make_program();
  std::vector<double> value((size_t)p.n_regs, 0.0);
  std::vector<double> adj((size_t)p.adj.n_regs, 0.0);
  value[0] = 1.125;
  value[1] = 1.75;
  double sink = 0.0;

  const double forward_ns = time_ns([&] {
    run_program(p, value.data());
    sink += value[(size_t)p.out_regs.back()];
  });

  run_program(p, value.data());
  const double reverse_ns = time_ns([&] {
    std::memset(adj.data(), 0, sizeof(double) * adj.size());
    for (int r : p.out_regs) adj[(size_t)p.adj.adj_reg[(size_t)r]] = 1.0;
    run_adjoint(p, p.adj, value.data(), adj.data());
    sink += adj[(size_t)p.adj.adj_reg[0]];
  });

  std::printf(
      "sites=%d reps=%d forward_ns=%.1f forward_ns_per_call=%.3f "
      "reverse_ns=%.1f reverse_ns_per_call=%.3f sink=%.17g\n",
      kSites, kReps, forward_ns, forward_ns / kSites, reverse_ns,
      reverse_ns / kSites, sink);
  return 0;
}
