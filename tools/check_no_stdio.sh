#!/usr/bin/env bash
# Builds stanli_runtime_objects with STANLI_NO_STDIO defined and fails if any
# of its object files (other than the ones stanr's tools/upgrade_stanli.sh
# deletes before packaging) reference an entry point R CMD check forbids in
# compiled code: the standard streams, abort/exit, or the assert failure path.
#
#   tools/check_no_stdio.sh [build-dir]     (default build-dir: build-no-stdio)
set -euo pipefail
cd "$(dirname "$0")/.."

BUILD_DIR=${1:-build-no-stdio}

# stanr deletes these source files before vendoring the runtime; their
# object files are not part of what ships and may still reference the
# symbols below.
EXEMPT_STEMS=(
  capi bridgestan_abi stanc_embed_c nuts estimate diagnose walnuts
  pathfinder data
)

# The entry points R CMD check rejects in compiled code (tools/sotools.R):
# the stdout/stderr objects and the functions that imply them, process
# exit, the assert failure path, and the C rand family. Writes to a FILE*
# the code opened itself are allowed.
FORBIDDEN=(
  stdout stderr __stdoutp __stderrp printf vprintf puts putchar
  abort exit _exit _Exit __assert_fail __assert_rtn
  rand srand random srandom
)

LAUNCHER_FLAG=()
if command -v ccache >/dev/null 2>&1; then
  LAUNCHER_FLAG=(-DCMAKE_CXX_COMPILER_LAUNCHER=ccache)
fi

cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-DSTANLI_NO_STDIO -DNDEBUG" \
  "${LAUNCHER_FLAG[@]}"
cmake --build "$BUILD_DIR" --target stanli_runtime_objects \
  --parallel "$(tools/build_jobs.sh)"

is_exempt() {
  local stem=$1 e
  for e in "${EXEMPT_STEMS[@]}"; do
    [ "$stem" = "$e" ] && return 0
  done
  return 1
}

# A symbol is forbidden if either the raw nm name or that name with one
# leading underscore stripped (the Mach-O C-symbol prefix; Linux ELF adds
# none) equals an entry verbatim, or if it demangles to std::cout/std::cerr
# (with or without libc++'s inline ABI namespace).
is_forbidden_symbol() {
  local raw=$1 demangled=$2 stripped f
  raw=${raw%%@@*}
  stripped=${raw#_}
  for f in "${FORBIDDEN[@]}"; do
    [ "$raw" = "$f" ] && return 0
    [ "$stripped" = "$f" ] && return 0
  done
  case "$demangled" in
    std::cout | std::__*::cout | std::cerr | std::__*::cerr) return 0 ;;
  esac
  return 1
}

found=0
while IFS= read -r -d '' obj; do
  stem=$(basename "$obj" .cpp.o)
  is_exempt "$stem" && continue

  raw_file=$(mktemp)
  demangled_file=$(mktemp)
  nm -uP "$obj" 2>/dev/null | awk '{print $1}' > "$raw_file"
  c++filt < "$raw_file" > "$demangled_file"

  while IFS=$'\t' read -r raw demangled; do
    [ -z "$raw" ] && continue
    if is_forbidden_symbol "$raw" "$demangled"; then
      echo "forbidden symbol in $obj: $demangled ($raw)" >&2
      found=1
    fi
  done < <(paste "$raw_file" "$demangled_file")
  rm -f "$raw_file" "$demangled_file"
done < <(find "$BUILD_DIR/CMakeFiles/stanli_runtime_objects.dir" -name '*.o' -print0)

if [ "$found" -ne 0 ]; then
  echo "check_no_stdio: forbidden symbols found under STANLI_NO_STDIO" >&2
  exit 1
fi
echo "check_no_stdio: stanli_runtime_objects has no forbidden symbols"
