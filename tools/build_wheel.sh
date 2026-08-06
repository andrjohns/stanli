#!/usr/bin/env bash
# Build the release wheel: optimized shared library, stripped, plus license
# notices. Release builds come from build-rel so a debug library can never
# ship by accident, and the platform tag names the architecture the library
# was actually built for (the default tag on macOS claims universal2, which
# would let the wheel install on machines it cannot run on).
set -euo pipefail
cd "$(dirname "$0")/.."

cmake --build build-rel -j8 --target stanli_shared

mkdir -p python/stanli/_bin
rm -f python/stanli/_bin/*
LIB=""
for cand in build-rel/libstanli.dylib build-rel/libstanli.so; do
  [ -f "$cand" ] && LIB=$cand && break
done
[ -n "$LIB" ] || { echo "no shared library in build-rel/"; exit 1; }
cp "$LIB" python/stanli/_bin/
strip -x "python/stanli/_bin/$(basename "$LIB")" 2>/dev/null || true
cp THIRD_PARTY_LICENSES.md LICENSE python/stanli/

# Without the embedded compiler the package needs the stanc binary instead.
if [ ! -f deps/stanc3/stanc_embed.o ]; then
  cp deps/stanc3/stanc python/stanli/_bin/stanc
  chmod +x python/stanli/_bin/stanc
fi

case "$(uname -s)-$(uname -m)" in
  Darwin-arm64) PLAT=macosx_11_0_arm64 ;;
  Darwin-x86_64) PLAT=macosx_10_15_x86_64 ;;
  Linux-aarch64) PLAT=manylinux_2_28_aarch64 ;;
  Linux-x86_64) PLAT=manylinux_2_28_x86_64 ;;
  *) echo "unknown platform: $(uname -s)-$(uname -m)"; exit 1 ;;
esac

rm -rf dist build/lib build/bdist.* python/build python/*.egg-info
(cd python && python3 setup.py -q bdist_wheel --plat-name "$PLAT" \
    --dist-dir ../dist)
rm -rf python/build python/*.egg-info
ls -la dist/
