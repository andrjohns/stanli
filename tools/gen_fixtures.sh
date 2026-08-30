#!/usr/bin/env bash
# Regenerate MIR fixtures from .stan sources with the pinned stanc3.
set -euo pipefail
cd "$(dirname "$0")/.."
stanc=${STANC:-./deps/stanc3/stanc}
exec python3 tools/gen_fixtures.py --stanc "$stanc"
