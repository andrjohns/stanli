// The webR runtime artifact loads into webR and resolves the C API.
//
// This is the ABI question the emsdk pin exists to answer: a side module
// built against the wrong Emscripten fails right here, in dyn.load. The
// full R-package flow (r-universe bridge, log_prob_grad, sampling) was
// verified by hand against webR 0.6.0 when this landed; this test keeps
// the load-and-resolve half, which needs nothing beyond the npm webr
// package.
//
// When the optional compiler is supplied, load that exact artifact through
// webR's host-JavaScript bridge and exercise its portable result as well.
//
// Usage: node tests/test_webr.mjs build-wasm-side/libstanli.so \
//          browser-compilers/stanli-compiler.js
import { WebR } from 'webr';
import fs from 'node:fs';

const soPath = process.argv[2];
const compilerPath = process.argv[3];
if (!soPath) {
  console.error(
      'usage: node tests/test_webr.mjs <libstanli.so> [stanli-compiler.js]');
  process.exit(2);
}

const webR = new WebR();
await webR.init();
const arch = (await (await webR.evalR('R.version$arch')).toJs()).values[0];
if (arch !== 'wasm32') {
  console.error(`FAIL unexpected webR arch ${arch}`);
  process.exit(1);
}

await webR.FS.writeFile('/tmp/libstanli.so',
                        new Uint8Array(fs.readFileSync(soPath)));

// The symbols the R bridge dlsyms, stanli_abi_version first: it is the
// handshake, and the one a hand-kept export list forgot once already.
const res = await webR.evalR(`
  dyn.load('/tmp/libstanli.so')
  syms <- c('stanli_abi_version', 'stanli_model_new', 'stanli_exact_lp',
            'stanli_grad', 'stanli_sample_multi', 'stanli_optimize',
            'stanli_diagnose_text', 'stanli_wa_row')
  syms[!vapply(syms, is.loaded, logical(1))]
`);
const missing = (await res.toJs()).values;
if (missing.length > 0) {
  console.error('FAIL symbols missing from the side module:', missing.join(' '));
  process.exit(1);
}

if (compilerPath) {
  await webR.FS.writeFile(
      '/tmp/stanli-compiler.js',
      new Uint8Array(fs.readFileSync(compilerPath)));
  await webR.FS.writeFile(
      '/tmp/portable-unicode.stan',
      new Uint8Array(fs.readFileSync('tests/compiler/portable_unicode.stan')));
  await webR.FS.writeFile(
      '/tmp/portable-bad.stan',
      new TextEncoder().encode('parameters { real x } model {}'));

  // This is the same mechanism mir_from_webr() uses in the R package. Keep
  // source and MIR in the shared filesystem, both to cover that transport and
  // to avoid marshalling a multi-megabyte document through evalR().
  const candidate = await webR.evalR(String.raw`
    eval_js <- get0("eval_js", envir = asNamespace("webr"))
    if (!is.function(eval_js)) stop("webR has no eval_js host bridge")
    load_compiler <- function() {
      # A browser worker has no Node process global. The npm webR harness runs
      # its worker under Node, where js_of_ocaml would otherwise select a
      # CommonJS filesystem and call an unavailable require(). Mask that test
      # runner detail while evaluating the exact browser artifact.
      eval_js('globalThis.__stanli_test_process = {
        present: Object.prototype.hasOwnProperty.call(globalThis, "process"),
        value: globalThis.process
      }; globalThis.process = undefined; "ok"')
      on.exit(eval_js('if (globalThis.__stanli_test_process.present) {
        globalThis.process = globalThis.__stanli_test_process.value;
      } else {
        delete globalThis.process;
      }
      delete globalThis.__stanli_test_process;
      "ok"'), add = TRUE)
      eval_js(paste(readLines("/tmp/stanli-compiler.js", warn = FALSE),
                    collapse = "\n"))
    }
    load_compiler()
    good <- eval_js('(() => {
      const f = globalThis.stanli_compile;
      if (typeof f !== "function") return "FAIL no stanli_compile()";
      const src = Module.FS.readFile("/tmp/portable-unicode.stan",
                                     {encoding: "utf8"});
      const r = f("webr_candidate", src);
      if (r.errors) return "FAIL valid source reported errors";
      const mir = String(r.result);
      if (!mir.startsWith("{\\\"stanli_ir\\\":1,\\\"program\\\":"))
        return "FAIL missing portable envelope";
      if (!mir.includes("π ☃ é 👋")) return "FAIL UTF-8 changed";
      Module.FS.writeFile("/tmp/portable-candidate.mir", mir);
      return "ok";
    })()')
    bad <- eval_js('(() => {
      const src = Module.FS.readFile("/tmp/portable-bad.stan",
                                     {encoding: "utf8"});
      const r = globalThis.stanli_compile("webr_bad", src);
      return (r.errors && typeof r.result === "undefined")
        ? "ok" : "FAIL malformed source produced a document";
    })()')
    mir <- readLines("/tmp/portable-candidate.mir", warn = FALSE)
    c(good, bad, if (length(mir) > 0L) "ok" else "FAIL MIR file is empty")
  `);
  const statuses = (await candidate.toJs()).values;
  const failure = statuses.find((status) => status !== 'ok');
  if (failure) {
    console.error(failure);
    process.exit(1);
  }
}

console.log(compilerPath
  ? 'webr load OK: runtime symbols and portable compiler resolve'
  : 'webr load OK: side module loads and the bridge symbols resolve');
process.exit(0);
