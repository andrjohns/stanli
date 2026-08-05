#!/usr/bin/env bash
# Build the platform wheel: cmake shared lib + stanc binary into package
# data, then pip wheel.
set -euo pipefail
cd "$(dirname "$0")/.."
cmake --build build -j8 --target stanrt_shared
mkdir -p python/stanrt/_bin
cp build/libstanrt.dylib python/stanrt/_bin/ 2>/dev/null || \
  cp build/libstanrt.so python/stanrt/_bin/
cp deps/stanc3/stanc python/stanrt/_bin/stanc
chmod +x python/stanrt/_bin/stanc
python3 -m pip wheel ./python -w dist --no-deps -q
ls -la dist/
