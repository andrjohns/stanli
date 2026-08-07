// The sampling worker: stanc3 (js_of_ocaml) compiles the model to MIR,
// stanli.wasm lowers it and runs NUTS. Everything heavy lives here so the
// page never blocks. Protocol: {cmd: "run", code, dataJson, seed, warmup,
// samples, delta} in; {status} progress messages and one {done} or
// {error} out.
"use strict";
importScripts("stancjs.bc.js", "stanli.js");

const ready = createStanli();

onmessage = async (e) => {
  const req = e.data;
  const t0 = performance.now();
  const say = (status) => postMessage({ status });
  try {
    const M = await ready;
    say("compiling Stan -> MIR (stanc3)");
    const sc = stanc("browser_model", req.code, ["debug-transformed-mir"]);
    if (sc.errors) throw new Error(Array.from(sc.errors).join("\n"));
    const tStanc = performance.now();

    say("lowering MIR -> op graph");
    const mirPtr = M.stringToNewUTF8(String(sc.result));
    const dataPtr = M.stringToNewUTF8(req.dataJson || "{}");
    const errLen = 8192;
    const errPtr = M._malloc(errLen);
    const model = M._stanli_model_new(mirPtr, dataPtr, errPtr, errLen);
    M._free(mirPtr);
    M._free(dataPtr);
    if (!model) throw new Error(M.UTF8ToString(errPtr));
    const tCompile = performance.now();

    const n = Number(M._stanli_n_unconstrained(model));
    const samples = req.samples | 0;
    const warmup = req.warmup | 0;
    say("NUTS: " + warmup + " warmup + " + samples + " draws");
    const drawsPtr = M._malloc(8 * samples * n);

    // Stream: every draw lands in the buffer before its callback, so the
    // page can plot the chain as it grows. Constrained rows batch in
    // chunks of 20 to keep message traffic sane.
    const nLive = Number(M._stanli_n_constrained(model));
    const liveNames = [];
    for (let i = 0; i < nLive; ++i)
      liveNames.push(
          M.UTF8ToString(M._stanli_constrained_name(model, BigInt(i))));
    postMessage({ liveMeta: { names: liveNames, warmup, samples } });
    const liveRowPtr = M._malloc(8 * nLive);
    let pend = [];
    const cbPtr = M.addFunction((i, wu) => {
      if (wu) {
        if (i % 25 === 0 || i + 1 === warmup)
          postMessage({ live: { phase: "warmup", i: i + 1 } });
        return;
      }
      M._stanli_constrain(model, drawsPtr + 8 * i * n, liveRowPtr);
      const row = new Float64Array(nLive);
      row.set(M.HEAPF64.subarray(liveRowPtr / 8, liveRowPtr / 8 + nLive));
      pend.push(row);
      if (pend.length >= 20 || i + 1 === samples) {
        const chunk = new Float64Array(pend.length * nLive);
        pend.forEach((r, k) => chunk.set(r, k * nLive));
        pend = [];
        postMessage({ live: { phase: "sampling", i: i + 1, nCon: nLive,
                              rows: chunk.buffer } }, [chunk.buffer]);
      }
    }, "viii");
    const rc = M._stanli_sample_stream(model, req.seed >>> 0, warmup,
                                       samples, +req.delta, drawsPtr,
                                       cbPtr, 0, errPtr, errLen);
    M.removeFunction(cbPtr);
    M._free(liveRowPtr);
    if (rc !== 0) throw new Error(M.UTF8ToString(errPtr));
    const tSample = performance.now();

    say("computing CSV columns");
    // write_array supplies every CmdStan CSV column (constrained params,
    // transformed parameters, generated quantities with seeded RNG);
    // models without a generate_quantities section fall back to the
    // constrained view.
    const nWa = Number(M._stanli_wa_n_columns(model));
    const useWa = nWa > 0;
    const nCon = useWa ? nWa : Number(M._stanli_n_constrained(model));
    const names = [];
    for (let i = 0; i < nCon; ++i)
      names.push(M.UTF8ToString(
          useWa ? M._stanli_wa_column_name(model, BigInt(i))
                : M._stanli_constrained_name(model, BigInt(i))));
    if (useWa) M._stanli_wa_seed(model, req.seed >>> 0);
    const cols = new Float64Array(nCon * samples);
    const rowPtr = M._malloc(8 * nCon);
    for (let s = 0; s < samples; ++s) {
      if (useWa) {
        if (M._stanli_wa_row(model, drawsPtr + 8 * s * n, rowPtr) !== 0)
          throw new Error("write_array failed on draw " + s);
      } else {
        M._stanli_constrain(model, drawsPtr + 8 * s * n, rowPtr);
      }
      for (let i = 0; i < nCon; ++i)
        cols[i * samples + s] = M.HEAPF64[rowPtr / 8 + i];
    }
    M._free(rowPtr);
    M._free(drawsPtr);
    M._free(errPtr);
    M._stanli_model_free(model);

    postMessage({
      done: {
        names, samples,
        columns: cols.buffer,
        ms: {
          stanc: tStanc - t0,
          lower: tCompile - tStanc,
          sample: tSample - tCompile,
          total: performance.now() - t0,
        },
      },
    }, [cols.buffer]);
  } catch (err) {
    postMessage({ error: String(err && err.message || err) });
  }
};
