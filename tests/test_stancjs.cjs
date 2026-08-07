// The browser compiler path: stancjs (js_of_ocaml build of the pinned
// stanc3) must emit the same transformed MIR as the native stanc binary,
// byte for byte, since that text is the wasm module's input contract.
//
//   node tests/test_stancjs.cjs [path/to/stancjs.bc.js]
"use strict";
const { execFileSync } = require("child_process");
const fs = require("fs");
const path = require("path");

const repo = path.resolve(__dirname, "..");
const jsPath = path.resolve(
    process.argv[2] ||
    path.join(repo, "deps", "stanc3-src", "_build", "default", "src",
              "stancjs", "stancjs.bc.js"));

// js_of_ocaml attaches Js.export to module.exports under CommonJS and to
// globalThis in a browser.
const exported = require(jsPath);
const stanc = (exported && exported.stanc) || globalThis.stanc;
if (typeof stanc !== "function") {
  console.error("FAIL stancjs did not export stanc()");
  process.exit(1);
}

const model = path.join(repo, "tests", "fixtures", "es.stan");
const code = fs.readFileSync(model, "utf8");

// The CLI names the model <stem>_model; match it so prog_name agrees.
const js = stanc("es_model", code, ["debug-transformed-mir"]);
if (js.errors) {
  console.error("FAIL stancjs errors: " + js.errors);
  process.exit(1);
}
const native = execFileSync(path.join(repo, "deps", "stanc3", "stanc"),
                            ["--debug-transformed-mir", model],
                            { maxBuffer: 1 << 28, encoding: "utf8" });

// stancjs returns the MIR without the trailing newline the CLI prints,
// and prog_path is CLI-only metadata (the absolute file path); neither
// reaches the lowering.
const strip = (s) => s.trim().replace(/\(prog_path [^)]*\)/, "(prog_path)");
const a = strip(String(js.result));
const b = strip(native);
if (a !== b) {
  const n = Math.min(a.length, b.length);
  let i = 0;
  while (i < n && a[i] === b[i]) ++i;
  console.error("FAIL MIR differs at byte " + i + ":\n  js:     ..." +
                a.slice(Math.max(0, i - 40), i + 40) + "\n  native: ..." +
                b.slice(Math.max(0, i - 40), i + 40));
  process.exit(1);
}
console.log("test_stancjs OK  (" + a.length + " bytes of MIR, identical)");
