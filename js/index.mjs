// stanli: full Stan in the browser. stanc3 (compiled to JS) turns Stan
// source into MIR, and a WASM build of the stanli runtime lowers it to an
// op graph and runs NUTS. Everything happens client side, off the main
// thread, in a worker this module owns.
//
//   import { sample } from "stanli";
//   const fit = await sample({ code, data: { N: 8, ... }, seed: 1 });
//   fit.columns["mu"]   // Float64Array of draws
//
// One worker is created on first use and reused; requests queue and run
// one at a time (the runtime is single threaded by design).

let worker = null;
let queue = Promise.resolve();

function getWorker() {
  if (!worker)
    worker = new Worker(new URL("./worker.js", import.meta.url));
  return worker;
}

/** Compile a Stan model and draw from its posterior.
 *
 * @param {Object} opts
 * @param {string} opts.code       Stan source.
 * @param {Object|string} [opts.data]  Data as an object or JSON text.
 * @param {number} [opts.seed=1]       Chain seed (sampler and GQ RNG).
 * @param {number} [opts.warmup=1000]
 * @param {number} [opts.samples=1000]
 * @param {number} [opts.delta=0.8]    Adaptation target acceptance.
 * @param {function(string)} [opts.onProgress]  Stage announcements.
 * @returns {Promise<{names: string[], samples: number,
 *                    columns: Object<string, Float64Array>,
 *                    ms: {stanc: number, lower: number, sample: number,
 *                         total: number}}>}
 *   One column per CSV column CmdStan would write: constrained
 *   parameters, transformed parameters, and generated quantities.
 */
export function sample(opts) {
  const run = () => new Promise((resolve, reject) => {
    const w = getWorker();
    w.onmessage = (e) => {
      const m = e.data;
      if (m.status) {
        if (opts.onProgress) opts.onProgress(m.status);
        return;
      }
      w.onmessage = null;
      w.onerror = null;
      if (m.error) {
        reject(new Error(m.error));
        return;
      }
      const { names, samples, ms } = m.done;
      const flat = new Float64Array(m.done.columns);
      const columns = {};
      names.forEach((name, i) => {
        columns[name] = flat.subarray(i * samples, (i + 1) * samples);
      });
      resolve({ names, samples, columns, ms });
    };
    w.onerror = (e) => {
      w.onmessage = null;
      w.onerror = null;
      reject(new Error("stanli worker: " + (e.message || "failed to load")));
    };
    w.postMessage({
      cmd: "run",
      code: opts.code,
      dataJson: typeof opts.data === "string"
          ? opts.data
          : JSON.stringify(opts.data || {}),
      seed: opts.seed == null ? 1 : opts.seed,
      warmup: opts.warmup == null ? 1000 : opts.warmup,
      samples: opts.samples == null ? 1000 : opts.samples,
      delta: opts.delta == null ? 0.8 : opts.delta,
    });
  });
  const p = queue.then(run);
  queue = p.catch(() => {});
  return p;
}
