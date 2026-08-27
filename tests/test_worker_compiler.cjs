// Exercise the worker's compiler selection without loading the WASM runtime.
// importScripts is replaced with a loader for the actual generated compiler
// artifacts; the compile command needs only the runtime's resolved promise.
//
//   node tests/test_worker_compiler.cjs custom worker.js portable.js stock.js
//   node tests/test_worker_compiler.cjs fallback worker.js portable.js stock.js
"use strict";

const fs = require("fs");
const path = require("path");

const mode = process.argv[2];
const workerPath = path.resolve(process.argv[3]);
const portablePath = path.resolve(process.argv[4]);
const stockPath = path.resolve(process.argv[5]);

if (mode !== "custom" && mode !== "fallback") {
  console.error("mode must be custom or fallback");
  process.exit(2);
}

const imports = [];
const messages = [];

globalThis.importScripts = (asset) => {
  imports.push(asset);
  if (asset === "stanli.js") {
    globalThis.createStanli = () => Promise.resolve({});
    return;
  }
  if (asset === "stanli-compiler.js") {
    if (mode === "fallback") throw new Error("portable compiler unavailable");
    Object.assign(globalThis, require(portablePath));
    return;
  }
  if (asset === "stancjs.bc.js") {
    Object.assign(globalThis, require(stockPath));
    return;
  }
  throw new Error("unexpected importScripts asset: " + asset);
};
globalThis.postMessage = (message) => messages.push(message);
// DedicatedWorkerGlobalScope provides this binding before the worker script
// runs. Define the corresponding Node global for the harness.
globalThis.onmessage = null;

require(workerPath);

const code = fs.readFileSync(
    path.join(__dirname, "fixtures", "es.stan"), "utf8");

Promise.resolve(globalThis.onmessage({data: {cmd: "compile", code}}))
    .then(() => {
      const completed = messages.filter((message) => message.done);
      if (completed.length !== 1 || typeof completed[0].done.mir !== "string")
        throw new Error("worker did not return one compiled MIR document");

      const mir = completed[0].done.mir;
      const portable = mir.startsWith("STANLI2:");
      if ((mode === "custom") !== portable)
        throw new Error(mode + " selected the wrong MIR producer");

      const expected = mode === "custom" ?
          ["stanli.js", "stanli-compiler.js"] :
          ["stanli.js", "stanli-compiler.js", "stancjs.bc.js"];
      if (JSON.stringify(imports) !== JSON.stringify(expected))
        throw new Error("unexpected import order: " + JSON.stringify(imports));

      console.log("test_worker_compiler " + mode + " OK");
    })
    .catch((error) => {
      console.error("FAIL " + String(error && error.stack || error));
      process.exit(1);
    });
