// Packet (varmat-style) kernel math.
//
// CmdStan's default AoS `Matrix<var>` rev overloads evaluate their
// transcendentals over strided `.val()` expressions that Eigen cannot
// packet-vectorize, so they run scalar libm per element and reduce
// sequentially. The kernels mirrored that exactly, which bought bitwise
// parity with default CmdStan at the cost of leaving Eigen's packet math
// unused. stan-math's varmat overloads (`var_value<Matrix>`, which
// `stanc --O1` emits wherever a variable's whole use chain allows it)
// compute over contiguous doubles instead and do vectorize.
//
// Enabling packet math here is the same trade in stanli's terms: the
// reference becomes `stanc --O1` CmdStan rather than default CmdStan, and
// affected gradients move from bitwise to a low-ULP match. Measured gains
// per element (tools/bench_packet.cpp, Apple M-series): exp 2.05x,
// inv_logit 1.93x, sum 4.3x, log 1.25x.
//
// On by default. Set STANLI_PACKET_MATH=0 for the scalar-libm arithmetic,
// which is what the corpus rig uses to report both accuracy tiers.
#ifndef STANLI_PACKET_HPP
#define STANLI_PACKET_HPP

namespace stanli {

// Read once from the environment on first call.
bool packet_math();

// Test-only override. The fixture tests that pin bitwise/low-ULP parity
// with DEFAULT CmdStan are assertions about the AoS mirror specifically,
// so they turn packet math off rather than widen their tolerance; the
// packet path's reference is `stanc --O1` CmdStan, checked separately.
void set_packet_math(bool on);

}  // namespace stanli

#endif
