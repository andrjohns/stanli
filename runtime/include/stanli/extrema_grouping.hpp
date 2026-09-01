// Small helpers for replaying Eigen's packet phase after a graph value has
// been materialized into a contiguous slot.
#ifndef STANLI_EXTREMA_GROUPING_HPP
#define STANLI_EXTREMA_GROUPING_HPP

#include <stan/math.hpp>

#include <cstdint>

namespace stanli {

// Bytes Eigen wants a double packet aligned to, or zero when this build has
// no packet reduction for double.
inline constexpr std::uintptr_t extrema_packet_alignment() {
  return static_cast<std::uintptr_t>(
      Eigen::internal::unpacket_traits<
          typename Eigen::internal::packet_traits<double>::type>::alignment);
}

// Number of doubles by which a contiguous view's offset changes the first
// aligned packet. One means every offset is already lane zero.
inline constexpr int64_t extrema_phase_modulus() {
  return extrema_packet_alignment() > sizeof(double)
             ? static_cast<int64_t>(extrema_packet_alignment() / sizeof(double))
             : 1;
}

inline int64_t extrema_phase_scratch(int64_t len) {
  return len + 2 * extrema_phase_modulus() + 1;
}

// Put values at the same packet phase as a view beginning `offset` elements
// into an aligned owning Eigen container, then let Stan Math perform the
// reduction. The caller supplies extrema_phase_scratch(len) doubles.
inline double extrema_phased(const double* data, int64_t len, int64_t offset,
                             bool maximum, double* scratch) {
  const int64_t modulus = extrema_phase_modulus();
  if (modulus <= 1 || scratch == nullptr) {
    const Eigen::Map<const Eigen::VectorXd> input(data, len);
    return maximum ? stan::math::max(input) : stan::math::min(input);
  }
  double* aligned = scratch;
  while (reinterpret_cast<std::uintptr_t>(aligned) %
             extrema_packet_alignment() !=
         0)
    ++aligned;
  double* phased = aligned + offset % modulus;
  for (int64_t i = 0; i < len; ++i) phased[i] = data[i];
  const Eigen::Map<const Eigen::VectorXd> input(phased, len);
  return maximum ? stan::math::max(input) : stan::math::min(input);
}

// Product analogue of extrema_phased. The caller supplies the same sized
// replay buffer so executor-driven products do not allocate per evaluation.
inline double prod_phased(const double* data, int64_t len, int64_t offset,
                          double* scratch) {
  if (extrema_phase_modulus() <= 1 || scratch == nullptr) {
    const Eigen::Map<const Eigen::VectorXd> input(data, len);
    return stan::math::prod(input);
  }
  double* aligned = scratch;
  while (reinterpret_cast<std::uintptr_t>(aligned) %
             extrema_packet_alignment() !=
         0)
    ++aligned;
  double* phased = aligned + offset % extrema_phase_modulus();
  for (int64_t i = 0; i < len; ++i) phased[i] = data[i];
  const Eigen::Map<const Eigen::VectorXd> input(phased, len);
  return stan::math::prod(input);
}

}  // namespace stanli

#endif
