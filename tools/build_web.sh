#!/usr/bin/env bash
# Assemble the browser demo in web/: build stanli.wasm (emsdk), build
# stancjs (opam switch with js_of_ocaml), copy both next to index.html.
# Serve with: python3 -m http.server -d web
set -euo pipefail
cd "$(dirname "$0")/.."

source deps/emsdk/emsdk_env.sh >/dev/null 2>&1
emcmake cmake -B build-wasm -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build-wasm -j8 --target stanli_wasm

if [ ! -f deps/stanc3-src/_build/default/src/stancjs/stancjs.bc.js ]; then
  (cd deps/stanc3-src && eval "$(opam env --switch=stanc3-55)" \
    && dune build src/stancjs/stancjs.bc.js)
fi

cp build-wasm/stanli.js build-wasm/stanli.wasm web/
cp deps/stanc3-src/_build/default/src/stancjs/stancjs.bc.js web/
ls -la web/
