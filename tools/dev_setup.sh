#!/usr/bin/env bash
# One-shot dev environment setup. Safe to re-run; every step is
# idempotent and skipped once its output exists.
#
#   tools/dev_setup.sh            core: vendored deps + cmake builds + tests
#   tools/dev_setup.sh --embed    + OCaml toolchain and in-process stanc3
#   tools/dev_setup.sh --corpus   + posteriordb and the CmdStan verify rig
#   tools/dev_setup.sh --all      everything
#
# Core needs: git, curl, cmake, a C++17 clang, python3.
# --embed adds: opam (OCaml 5.5.0 switch built automatically).
# --corpus adds: ~2 GB of checkouts under deps/ and a CmdStan build.
set -euo pipefail
cd "$(dirname "$0")/.."
REPO=$PWD

STANC3_SRC_SHA=ac69570adecc41925b4ad72b6b2681c98c09c57d  # matches deps/stanc3/stanc
PDB_SHA=28f8d3d6e975315f42aa274a8399f21e07a43b30
CMDSTAN_SHA=11cb052d3e1fc8c799e0fec559e2ee5452b38d27
OPAM_SWITCH=stanc3-55
OCAML_VERSION=5.5.0

WANT_EMBED=0
WANT_CORPUS=0
for arg in "$@"; do
  case "$arg" in
    --embed) WANT_EMBED=1 ;;
    --corpus) WANT_CORPUS=1 ;;
    --all) WANT_EMBED=1; WANT_CORPUS=1 ;;
    -h|--help) sed -n '2,12p' "$0"; exit 0 ;;
    *) echo "unknown flag: $arg (try --help)"; exit 2 ;;
  esac
done

step() { printf '\n== %s\n' "$*"; }
have() { command -v "$1" >/dev/null 2>&1; }

# --- host prerequisites ----------------------------------------------------
step "checking prerequisites"
missing=()
for tool in git curl cmake python3; do have "$tool" || missing+=("$tool"); done
if ! have clang++ && ! have g++; then missing+=("clang++ (Xcode CLT or clang)"); fi
if [ "$WANT_EMBED" = 1 ] && ! have opam; then missing+=(opam); fi
if [ ${#missing[@]} -gt 0 ]; then
  if have brew; then
    echo "installing via homebrew: ${missing[*]}"
    for tool in "${missing[@]}"; do
      case "$tool" in
        cmake|opam|git|curl) brew install "$tool" ;;
        *) echo "install manually: $tool"; exit 1 ;;
      esac
    done
  else
    echo "missing: ${missing[*]}"
    echo "install them (apt: sudo apt install ${missing[*]}) and re-run."
    exit 1
  fi
fi
echo "ok: git curl cmake python3 and a C++ compiler present"

# --- vendored headers + stanc binary ---------------------------------------
step "fetching pinned deps (stan-math, stan, stanc3 binary)"
./deps/fetch.sh

# --- embedded stanc3 (optional) --------------------------------------------
if [ "$WANT_EMBED" = 1 ]; then
  step "OCaml toolchain for the embedded stanc3"
  if [ ! -d "$HOME/.opam" ]; then opam init -y --bare; fi
  eval "$(opam env 2>/dev/null || true)"
  if ! opam switch list --short 2>/dev/null | grep -qx "$OPAM_SWITCH"; then
    # stanc3 pins its OCaml version exactly; other versions fail to solve.
    opam switch create "$OPAM_SWITCH" "ocaml-base-compiler.$OCAML_VERSION" -y
  fi

  step "stanc3 source at $STANC3_SRC_SHA"
  if [ ! -d deps/stanc3-src/.git ]; then
    git clone https://github.com/stan-dev/stanc3.git deps/stanc3-src
  fi
  git -C deps/stanc3-src fetch -q origin "$STANC3_SRC_SHA"
  git -C deps/stanc3-src checkout -q "$STANC3_SRC_SHA"

  step "stanc3 OCaml dependencies (switch $OPAM_SWITCH)"
  (cd deps/stanc3-src &&
   opam install . --switch="$OPAM_SWITCH" --deps-only -y)

  step "building the embeddable stanc object"
  if [ ! -f deps/stanc3/stanc_embed.o ]; then
    tools/stanc_embed/build.sh deps/stanc3-src "$OPAM_SWITCH"
  else
    echo "deps/stanc3/stanc_embed.o already present; delete it to rebuild"
  fi
fi

# --- cmake builds ----------------------------------------------------------
step "configuring and building (build/ dev, build-rel/ benchmarks)"
EMBED_FLAGS=()
if [ -f deps/stanc3/stanc_embed.o ] && have opam; then
  EMBED_FLAGS=(
    "-DSTANRT_STANC_EMBED_OBJ=$REPO/deps/stanc3/stanc_embed.o"
    "-DSTANRT_OCAML_STDLIB=$(opam var --switch="$OPAM_SWITCH" lib 2>/dev/null)/ocaml"
  )
fi
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  ${EMBED_FLAGS[@]+"${EMBED_FLAGS[@]}"}
cmake --build build -j8
cmake -B build-rel -DCMAKE_BUILD_TYPE=Release
cmake --build build-rel -j8 --target bench_grad stanrt_run

step "running tests"
ctest --test-dir build --output-on-failure

# --- corpus + differential verification rig (optional) ---------------------
if [ "$WANT_CORPUS" = 1 ]; then
  step "posteriordb at $PDB_SHA (deps/posteriordb)"
  if [ ! -d deps/posteriordb/.git ]; then
    git clone https://github.com/stan-dev/posteriordb.git deps/posteriordb
  fi
  git -C deps/posteriordb fetch -q origin "$PDB_SHA"
  git -C deps/posteriordb checkout -q "$PDB_SHA"

  step "CmdStan at $CMDSTAN_SHA (deps/cmdstan; used by tools/verify_sample.py)"
  if [ ! -d deps/cmdstan/.git ]; then
    git clone https://github.com/stan-dev/cmdstan.git deps/cmdstan
  fi
  git -C deps/cmdstan fetch -q origin "$CMDSTAN_SHA"
  git -C deps/cmdstan checkout -q "$CMDSTAN_SHA"
  git -C deps/cmdstan submodule update --init --recursive --quiet

  if [ ! -f deps/cmdstan/stan/lib/stan_math/lib/tbb/libtbb.dylib ] &&
     [ ! -f deps/cmdstan/stan/lib/stan_math/lib/tbb/libtbb.so.2 ]; then
    step "building CmdStan (one-time; provides TBB + the bench comparator)"
    make -C deps/cmdstan -j8 build
  else
    echo "CmdStan already built"
  fi

  step "corpus scoreboard"
  python3 tools/corpus.py deps/posteriordb || true
fi

step "done"
echo "dev build:   build/            (tests: ctest --test-dir build)"
echo "bench build: build-rel/        (tools/bench_grad.cpp)"
echo "corpus:      python3 tools/corpus.py deps/posteriordb"
echo "verify:      python3 tools/verify_sample.py deps/cmdstan deps/posteriordb MODEL..."
echo "wheel:       tools/build_wheel.sh"
