// Nothing but an anchor for the shared precompiled header.
//
// CMake's REUSE_FROM needs a real target and source file to build the PCH. This
// must not be tests/tbb_stub.cpp: on non-Apple platforms that file defines a
// TBB compatibility class which <stan/math.hpp> has already declared.
namespace stanli {
namespace {
[[maybe_unused]] constexpr int kPchAnchor = 0;
}  // namespace
}  // namespace stanli
