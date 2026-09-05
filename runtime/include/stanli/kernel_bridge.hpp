#ifndef STANLI_KERNEL_BRIDGE_HPP
#define STANLI_KERNEL_BRIDGE_HPP

#include <stanli/optable.hpp>
#include <stanli/program.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace stanli {

template <typename T>
void call_kernel(uint16_t opcode, uint8_t variant, uint8_t input_adjoint_mask,
                 std::vector<int> idata,
                 const std::vector<const std::vector<T>*>& in,
                 std::vector<T>& out) {
  const Kernel* kernel = find_kernel(opcode);
  if (kernel == nullptr)
    throw std::logic_error(std::string("kernel_bridge: unavailable opcode ") +
                           opcode_name(opcode));
  Program::Call call;
  call.opcode = opcode;
  call.variant = variant;
  call.input_adjoint_mask = input_adjoint_mask;
  call.n_in = (int8_t)in.size();
  int32_t total = 0;
  for (size_t k = 0; k < in.size(); ++k) {
    call.in[k] = total;
    call.in_len[k] = (int32_t)in[k]->size();
    total += call.in_len[k];
  }
  call.out = total;
  call.out_len = (int32_t)out.size();
  total += call.out_len;
  call.idata = std::move(idata);
  const int64_t scratch = kernel_call_scratch(
      kernel->scratch_size, opcode, variant, call.n_in, call.in_len,
      call.out_len, call.idata.data(), (int64_t)call.idata.size(), nullptr);
  call.scratch = total;
  call.scratch_len = (int32_t)scratch;
  total += call.scratch_len;
  if (!bind_call(call))
    throw std::logic_error(std::string("kernel_bridge: unbound opcode ") +
                           opcode_name(opcode));

  std::vector<T> reg((size_t)total, T(0.0));
  for (size_t k = 0; k < in.size(); ++k)
    for (size_t i = 0; i < in[k]->size(); ++i)
      reg[(size_t)(call.in[k] + i)] = (*in[k])[i];

  if constexpr (std::is_same_v<T, double>) {
    run_call(call, reg.data(), nullptr);
  } else {
    run_call_var(call, reg.data());
  }
  for (int i = 0; i < call.out_len; ++i)
    out[(size_t)i] = reg[(size_t)(call.out + i)];
}

}  // namespace stanli

#endif
