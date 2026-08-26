# OCaml MIR backend rollout

This is the execution plan for moving stanli's source-sensitive MIR work into
OCaml while keeping data specialization, graph construction, kernels, and the
runtime in C++. It incorporates the Fable 5 Max review from 2026-08-26.

## Ownership boundary

```text
Stan source
  -> stanc3 parse/typecheck/Stan-Math transform/O1       OCaml, stan-dev/stanc3
  -> selected passes + Portable MIR encoder              OCaml, stanli
  -> Portable MIR v1 or legacy debug MIR decoder         C++, stanli
  -> data binding + transformed data + Graph lowering    C++, stanli
  -> Graph passes + executor + kernels + samplers        C++, stanli
```

The upstream interface is `Driver.Entry.stan2mir`. Upstream owns the typed MIR
and its standard passes. Stanli owns pass selection, any stanli-specific MIR
transform, the portable schema, every producer entry point, and both decoders.

The OCaml package exists only while compiling Stan source. No OCaml value
crosses the language boundary, OCaml does not run while sampling, and OCaml
does not emit the final data-specialized Graph.

The legacy decoder remains available for cached MIR, checked-in fixtures, and
users compiling with stock stanc3.

## Requirements carried from review

1. The encoder is total over `Middle.Program.Typed.t`. Unsupported forms are
   represented explicitly and fail at the normal unsupported boundary; they
   are never dropped from the document.
2. Portable MIR v1 is a faithful field-level re-tagging. No source idiom is
   normalized until a separate change proves that normalization against the
   corpus.
3. Native OCaml, js_of_ocaml, and Windows producers must emit byte-identical
   canonical UTF-8 for the same model name and source.
4. Producer artifacts are tied to the exact stanc3 revision, OCaml toolchain,
   js_of_ocaml version where applicable, and every stanli encoder input.
5. Source-sensitive optimization cannot turn on until every shipping channel
   uses the same stanli pipeline.

## Completed foundation

PR #208 completed the first tranche:

- upstream `stan2mir` merged and stanli pinned its merge commit;
- exact-source compiler builds replaced the moving Windows artifact;
- Portable MIR v1 and its canonical OCaml encoder landed;
- the strict portable decoder and legacy decoder both produce `mir::Program`;
- the embedded macOS/Linux producer emits portable JSON;
- semantic comparison covered 147 fixture programs and executable models;
- malformed portable inputs are structurally validated before lowering;
- native, Windows-stock, browser-stock, and R compiler provenance is checked.

## Phase 1: shared producer and browser/npm

Status: complete. All gates below run in CI.

- Move the encoder and O1 policy into one stanli-owned OCaml package under
  `compiler/`.
- Keep thin native, js_of_ocaml, and later Windows entry points around that
  package.
- Build `stanli-compiler.js` for the browser and npm package.
- Preserve the ordinary `stanc()` JavaScript API in that artifact.
- Have the worker prefer `stanli_compile()` and retain stock `stancjs.bc.js`
  as the rollback path for one release cycle.
- Leave the CRAN-bundled `r/inst/js/stanc.js` on the legacy format in this
  phase.
- Make every R legacy path request O1 optimized MIR rather than transformed
  but unoptimized MIR.
- Serialize embedded compiler calls and register foreign C/Python threads
  with the OCaml runtime.

Gates:

- Native and js_of_ocaml bytes agree for an ordinary model, a nested UDF, the
  mother model, an expression where O1 folds `0.1 + 0.2` to the exact binary64
  payload `3fd3333333333334`, checked int32 overflow/no-fold behavior, Unicode
  text, and an in-memory include.
- Repeated JS compilation is byte-identical.
- A malformed model produces an error-only result, never a partial document.
- Frontend diagnostics retain source excerpts, compiler warnings agree with
  stock stancjs, and JavaScript include maps reach the shared pipeline.
- The custom artifact's `stanc()` behavior agrees with stock stancjs on
  ordinary inputs; the checked overflow fixture deliberately differs from
  pristine stanc3's host-width-dependent fold.
- The custom compiler feeds the WASM runtime through gradient, NUTS, WALNUTS,
  Pathfinder, and generated-quantity paths.
- Compiler cache keys include all shared OCaml and entry-point sources.

Initial measurements on eight schools, Apple arm64 release builds:

| Item | Legacy | Portable |
|---|---:|---:|
| MIR bytes | 33,320 | 111,760 |
| MIR gzip bytes | 2,000 | 2,365 |
| decoder parse time, median of 5 | 0.290 ms | 2.799 ms |
| complete preparation, representative | 0.59 ms | 3.31 ms |

The custom JS compiler is 2,990,736 bytes / 425,026 gzip, compared with
2,971,677 bytes / 418,847 gzip for stock stancjs. Shipping both for the
rollback cycle doubles the compiler portion of the browser payload; removing
the stock producer after the rollback window recovers that temporary cost.

These are observations, not pass/fail thresholds. Source compilation still
dominates this one-time preparation path, but subsequent schema work should
avoid increasing the JSON multiplier further.

## Phase 2: Windows portable producer

Status: implemented. The compiler-only gate runs on pull requests; the full
Windows C++ build runs after merge, nightly, and on release tags.

- Cross-build pristine `stanc.exe` before applying the stanli overlay, then
  cross-build the shared CLI entry point as `stanli-compile.exe` with stanc3's
  Linux-to-Windows recipe.
- Ship both beside `stanli.dll` for one release cycle. The portable producer is
  preferred; pristine stanc is the rollback selected only when it is absent.
- Python searches its packaged `_bin` directory for `stanli-compile.exe`, then
  `stanc.exe`. Native R first honors the explicit `STANLI_STANC` stock override,
  then checks beside the runtime for the portable producer and stock stanc,
  then checks `PATH` for stock stanc, and finally uses its JavaScript path.
- Treat a selected compiler's launch error, nonzero exit, or empty output as
  an error. If the runtime decoder later rejects its document, surface that
  error too; do not hide either failure by retrying stock.
- Keep the Windows compiler in short-lived subprocesses. No OCaml compiler DLL
  is introduced.

Pull-request gates:

- Validate the exact stanc3 revision, the portable producer's source/toolchain
  stamp, LF checkout bytes for every stamped input, and that both artifacts are
  x86-64 PE executables.
- Execute pristine stanc on the checked overflow fixture to prove it was copied
  before the overlay changed the fold policy.
- Execute `stanli-compile.exe` on Windows and compare its output byte for byte
  with the same JavaScript producer already compared with native OCaml. The
  shared seven-model suite covers nested UDFs, the mother model, folded
  binary64, checked int32 overflow, Unicode, includes, and final-newline
  behavior. The surrounding JavaScript gate covers diagnostics, warnings,
  stock API compatibility, and repeat determinism.
- Run both executables through native R from paths containing spaces and
  Unicode, with CRLF source staged as exact UTF-8 bytes, and distinguish the
  portable and legacy envelopes.

Post-merge full-platform gates:

- Build the Windows C++ runtime and run CTest.
- Require the wheel and R runtime tarball to contain `stanli.dll`,
  `stanli-compile.exe`, and pristine `stanc.exe`.
- Install the wheel into python.org CPython and exercise source compilation,
  errors, lowering, gradients, sampling, and generated quantities.

This split keeps the bounded compiler contract merge-blocking while the
multi-hour stan-math build remains a post-merge platform check. Publishing
still waits for the full Windows wheel.

## Phase 3: CRAN and webR compiler

Status: compatibility gate not yet open. Stanli has not completed a CRAN
release, so the compiler carried by the package cannot switch formats yet. The
v0.9.2 release tarball is the baseline candidate: it carries the legacy
JavaScript producer and pins a runtime with both decoders. Submit that exact
artifact first, record its CRAN acceptance, and make the producer switch in a
later CRAN release.

Switch the compiler carried inside the R package only after a released R
runtime with the dual decoder has existed for at least one CRAN cycle.

Readiness checks land before that switch without changing the shipped path:

- The browser compiler artifact is provenance-checked, loaded directly into a
  fresh V8 context, and its portable output is passed through the public
  `stanli_model(mir=...)` API against the real Linux runtime. That check covers
  UTF-8, deterministic bytes, warnings, malformed input, gradients, sampling,
  and generated quantities.
- The webR side-module job loads the same compiler artifact through webR's
  host-JavaScript bridge and covers portable output plus malformed source.
- The R package test asserts that its bundled compiler still emits the legacy
  s-expression during the compatibility release.

- Build the tracked compiler JS from the shared package.
- Update V8 and webR to prefer portable output.
- Keep stock stanc and old-package compatibility through the legacy decoder.
- Byte-compare the tracked artifact with a fresh exact-source build.
- Exercise V8, native subprocess, and webR source-compilation paths.

This phase is last because the R package carries compiler JS while webR
downloads its runtime separately; users can therefore combine package and
runtime versions from different release dates.

## Phase 4: upstream `vectorize_loops`

The pinned stanc3 contains the pass, but O1 leaves it disabled. Enable O1 plus
only `vectorize_loops` in the shared stanli pipeline. Do not enable the entire
experimental suite.

Keep C++ reroll enabled initially and measure with the OCaml pass both on and
off:

- whole-corpus log density, gradient, write-array, and error parity;
- graph op counts and which C++ reroll patterns still fire;
- compile time and preparation time;
- interleaved gradient timings for `radon_pooled`, `arK`, and models whose
  loops should remain untouched;
- arithmetic-order changes, reported separately from semantic mismatches.

Only remove a C++ reroll case after the pass-off comparison shows that no
supported model still benefits from it.

## Phase 5: stanli-specific OCaml transformations

Add one structural, data-independent transformation per PR. Each transform
gets:

- a typed-MIR unit test;
- exact producer-byte tests on every toolchain;
- pass-on/pass-off corpus and conformance runs;
- op-count and performance measurements;
- a capability check for the runtime vocabulary it can synthesize.

Data-dependent transformations remain in C++: transformed-data evaluation,
concrete dimensions, data-known branches and loop bounds, physical layouts,
kernel selection, in-place rewrites, CSE, partitioning, tape islands, arena
packing, and executor scheduling.

## Phase 6: producer retirement

- Remove stock stancjs from browser/npm after the rollback cycle.
- Remove the duplicate stock Windows compiler after the Windows portable
  producer has shipped and downgrade behavior is documented.
- Deprecate legacy-producing convenience paths only after all maintained
  channels emit portable MIR.
- Retain the legacy decoder indefinitely.
- Consider explicit schema fields that replace old MIR idioms one at a time;
  each is a new gated schema evolution, not an implicit v1 normalization.
