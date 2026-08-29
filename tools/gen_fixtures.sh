#!/usr/bin/env bash
# Regenerate MIR fixtures from .stan sources with the pinned stanc3.
set -euo pipefail
cd "$(dirname "$0")/.."
# stanc does not terminate its last line, so add the newline here and keep
# every fixture reproducible byte for byte.
for f in tests/fixtures/*.stan; do
  { ./deps/stanc3/stanc --O1 --debug-optimized-mir "$f"; echo; } \
    > "${f%.stan}.tmir.sexp"
done
# stanc also emits C++ next to the model; fixtures only keep the MIR.
rm -f tests/fixtures/*.hpp
