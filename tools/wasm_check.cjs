// stanli_check's contract, driven through the WASM build under Node:
//   OK <lp> <g0> <g1> ...   / COMPILE_FAIL ... / EVAL_FAIL ...
// stanc runs natively (Node spawns the same pinned binary); the model
// compiles and evaluates inside stanli.wasm. This is what lets
// tools/verify_refs.py replay the corpus references against the browser
// build: tools/wasm_check.sh adapts the argv.
//
//   node tools/wasm_check.cjs model.stan data.json [--point N]
//
// The WASM C ABI has no write_array entry point yet, so --wa-values
// reports failure; replay wa-carrying models with --no-wa.
"use strict";
const { execFileSync } = require("child_process");
const fs = require("fs");
const path = require("path");

const repo = path.resolve(__dirname, "..");
const args = process.argv.slice(2);
const model = args[0], dataFile = args[1];
let point = 0, waValues = false;
for (let i = 2; i < args.length; ++i) {
  if (args[i] === "--point") point = parseInt(args[++i], 10);
  else if (args[i] === "--wa-values") waValues = true;
}

// Same deterministic points as stanli_check.cpp / ref_driver.cpp.
function evalPoint(i, variant) {
  switch (variant) {
    case 1: return 0.02 * ((i % 5) - 2);
    case 2: return 0.0;
    default: return 0.1 + 0.05 * (i % 7) - 0.15 * (i % 3);
  }
}

let mir;
try {
  mir = execFileSync(path.join(repo, "deps", "stanc3", "stanc"),
                     ["--debug-transformed-mir", model],
                     { maxBuffer: 1 << 28, encoding: "utf8" });
} catch (e) {
  console.log("COMPILE_FAIL stanc: " + String(e.message).split("\n")[0]);
  process.exit(1);
}
const data = fs.readFileSync(dataFile, "utf8");

const createStanli = require(path.join(repo, "build-wasm", "stanli.js"));
createStanli().then((M) => {
  const mirPtr = M.stringToNewUTF8(mir);
  const dataPtr = M.stringToNewUTF8(data);
  const errLen = 8192;
  const errPtr = M._malloc(errLen);
  const m = M._stanli_model_new(mirPtr, dataPtr, errPtr, errLen);
  if (!m) {
    console.log("COMPILE_FAIL " +
                M.UTF8ToString(errPtr).split("\n")[0]);
    process.exit(1);
  }
  const n = Number(M._stanli_n_unconstrained(m));
  const qPtr = M._malloc(8 * n);
  const lpPtr = M._malloc(8);
  const gradPtr = M._malloc(8 * n);
  for (let i = 0; i < n; ++i) M.HEAPF64[qPtr / 8 + i] = evalPoint(i, point);
  if (M._stanli_grad(m, qPtr, lpPtr, gradPtr) !== 0) {
    console.log("EVAL_FAIL evaluation threw");
    process.exit(1);
  }
  const lp = M.HEAPF64[lpPtr / 8];
  if (!Number.isFinite(lp)) {
    console.log("EVAL_FAIL nonfinite lp");
    process.exit(1);
  }
  const parts = ["OK", fmt(lp)];
  for (let i = 0; i < n; ++i) {
    const g = M.HEAPF64[gradPtr / 8 + i];
    if (!Number.isFinite(g)) {
      console.log("EVAL_FAIL nonfinite gradient");
      process.exit(1);
    }
    parts.push(fmt(g));
  }
  console.log(parts.join(" "));
  if (waValues)
    console.log("WANAMES FAIL wasm capi has no write_array\nWAVALS FAIL");
  process.exitCode = 0;
}).catch((e) => {
  console.log("EVAL_FAIL " + String(e).split("\n")[0]);
  process.exit(1);
});

// %.17g equivalent: shortest round-trip representation, which is what
// Number.prototype.toString gives for doubles.
function fmt(x) {
  return String(x);
}
