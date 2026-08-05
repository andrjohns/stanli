# stanrt

A portable Stan runtime: op-graph executor over precompiled stan-math
kernels. No C++ toolchain, no LLVM, no compilation on the user's machine.

Design: `docs/superpowers/specs/2026-08-04-stan-portable-runtime-design.md`
Current plan: `docs/superpowers/plans/2026-08-04-m1-spine.md`

## Build

```
./deps/fetch.sh
cmake -B build
cmake --build build -j
ctest --test-dir build
```

Status: milestone 1 in progress.
