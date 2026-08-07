#!/usr/bin/env bash
# stanli_check argv adapter for the WASM build: verify_refs.py --check
# takes one binary path, so this execs the Node driver.
exec node "$(dirname "$0")/wasm_check.cjs" "$@"
