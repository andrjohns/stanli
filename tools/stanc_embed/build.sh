#!/usr/bin/env bash
# Build the embeddable stanc object: copies the shim into a stanc3 checkout,
# builds with dune (-output-complete-obj), and drops stanc_embed.o into
# deps/stanc3/.
# Usage: tools/stanc_embed/build.sh /path/to/stanc3-src [opam-switch]
set -euo pipefail
cd "$(dirname "$0")/../.."
SRC=${1:?stanc3 source dir}
SWITCH=${2:-stanc3-55}
mkdir -p "$SRC/src/stanc_embed"
cp tools/stanc_embed/stanc_embed.ml tools/stanc_embed/dune "$SRC/src/stanc_embed/"
eval "$(opam env --switch="$SWITCH")"
(cd "$SRC" && dune build src/stanc_embed/stanc_embed.exe.o 2>&1 | tail -5 ||
 dune build src/stanc_embed 2>&1 | tail -5)
OBJ=$(find "$SRC/_build" -name "stanc_embed*.o" | head -1)
cp "$OBJ" deps/stanc3/stanc_embed.o
echo "embedded stanc object: deps/stanc3/stanc_embed.o ($(du -h deps/stanc3/stanc_embed.o | cut -f1))"
