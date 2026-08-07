#!/usr/bin/env bash
# Fetch pinned dependencies into deps/. Safe to re-run.
set -euo pipefail
cd "$(dirname "$0")"

MATH_SHA=8f326d14599d3030c626c46532d8e8534c1cdbec
STAN_SHA=c96d04115d35cb04f42e45c5a69a82f9704798f1

fetch() { # name url sha sparse-paths...
  local name=$1 url=$2 sha=$3
  shift 3
  if [ ! -d "$name/.git" ]; then
    git clone --filter=blob:none --no-checkout "$url" "$name"
    git -C "$name" sparse-checkout init --cone
  fi
  git -C "$name" sparse-checkout set "$@"
  git -C "$name" fetch -q origin "$sha"
  git -C "$name" checkout -q "$sha"
}

fetch math https://github.com/stan-dev/math.git "$MATH_SHA" stan lib
fetch stan https://github.com/stan-dev/stan.git "$STAN_SHA" src/stan

# stanc3 release binary (pinned nightly, stanc3 ac69570). Per-OS asset name.
STANC_ASSET=mac-arm64-stanc
case "$(uname -s)-$(uname -m)" in
  Darwin-arm64) STANC_ASSET=mac-arm64-stanc ;;
  Darwin-x86_64) STANC_ASSET=mac-stanc ;;
  Linux-aarch64) STANC_ASSET=linux-arm64-stanc ;;
  Linux-x86_64) STANC_ASSET=linux-stanc ;;
  MINGW*|MSYS*|CYGWIN*) STANC_ASSET=windows-stanc ;;
esac
mkdir -p stanc3
if [ ! -x stanc3/stanc ]; then
  curl -sL -o stanc3/stanc \
    "https://github.com/stan-dev/stanc3/releases/download/nightly/$STANC_ASSET"
  chmod +x stanc3/stanc
fi
./stanc3/stanc --version

echo "deps ready: math@$MATH_SHA stan@$STAN_SHA"
