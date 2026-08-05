#!/usr/bin/env bash
# Regenerate MIR fixtures from .stan sources with the pinned stanc3.
set -euo pipefail
cd "$(dirname "$0")/.."
for f in tests/fixtures/*.stan; do
  ./deps/stanc3/stanc --debug-transformed-mir "$f" > "${f%.stan}.tmir.sexp"
done
