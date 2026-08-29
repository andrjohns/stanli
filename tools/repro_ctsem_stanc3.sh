#!/usr/bin/env bash
# Reproduce the ctsem model-only stanc3 optimizer case from stanli issue #248.
# The GPL-3 model is fetched at an immutable ctsem revision rather than copied
# into this repository. No data file is needed: the problem is source-to-MIR.
set -euo pipefail

cd "$(dirname "$0")/.."

STANC=${1:-deps/stanc3/stanc}
OUT_DIR=${2:-$(mktemp -d "${TMPDIR:-/tmp}/stanli-ctsem-stanc3.XXXXXX")}
CTSEM_REV=a0f1e69b7282c1dcaed820919ae0d8b280d076c4
CTSEM_SHA256=b148f5b3f129f981e7a3bfbd966e825c28fbc7314d14e2896d18c7c6bc1b01d2
MODEL_URL="https://raw.githubusercontent.com/cdriveraus/ctsem/${CTSEM_REV}/inst/stan/ctsm.stan"
MODEL="$OUT_DIR/ctsm.stan"

mkdir -p "$OUT_DIR"
curl --fail --location --silent --show-error "$MODEL_URL" --output "$MODEL"
if command -v shasum >/dev/null 2>&1; then
  ACTUAL_SHA256=$(shasum -a 256 "$MODEL" | awk '{print $1}')
else
  ACTUAL_SHA256=$(sha256sum "$MODEL" | awk '{print $1}')
fi
if [ "$ACTUAL_SHA256" != "$CTSEM_SHA256" ]; then
  echo "ctsem model hash mismatch: $ACTUAL_SHA256" >&2
  exit 1
fi

case "$(uname -s)" in
  Darwin) TIME_ARGS=(-l) ;;
  Linux) TIME_ARGS=(-v) ;;
  *) TIME_ARGS=() ;;
esac

measure() {
  local name=$1
  shift
  local mir="$OUT_DIR/${name}.mir"
  local timing="$OUT_DIR/${name}.time"
  echo "+ /usr/bin/time ${TIME_ARGS[*]} $STANC $* $MODEL"
  /usr/bin/time "${TIME_ARGS[@]}" "$STANC" "$@" "$MODEL" \
    >"$mir" 2>"$timing"
  printf '%s: bytes=%s while_nodes=%s\n' "$name" \
    "$(wc -c <"$mir" | tr -d ' ')" \
    "$(grep -o 'While' "$mir" | wc -l | tr -d ' ')"
  sed -n '1,80p' "$timing"
}

echo "stanc: $($STANC --version)"
echo "ctsem revision: $CTSEM_REV"
echo "model SHA-256: $CTSEM_SHA256"
echo "artifacts: $OUT_DIR"
measure transformed --debug-transformed-mir
measure optimized --O1 --debug-optimized-mir
