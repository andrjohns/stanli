# Hacking on stanli

A map for contributors: which file owns what, and the recipes for the two
most common changes. `docs/how-it-works.md` explains why the design is
what it is; `runtime/src/OPTIMIZATIONS.md` explains the graph passes.

## Layout

| Path | Owns |
|---|---|
| `runtime/include/stanli/` | All public headers. `graph.hpp` is the IR: `Slot` + `Op` over flat arenas. |
| `runtime/src/lower.cpp` | The compiler: transformed MIR in, op graph out. `lower_expr`/`lower_stmt` walk statements; function calls dispatch through `lower_density_fn`, `lower_eltwise_fn`, `lower_matrix_fn`, `lower_ode_fn`. `lower_read_param` builds the constrain ops and the parameter views. |
| `runtime/src/mir_reader.cpp`, `sexp.hpp`, `mir.hpp` | Parse stanc3's `--debug-transformed-mir` s-expressions into the C++ MIR structs. Anything unrecognized is preserved as raw text and fails loudly if reached. |
| `runtime/src/inplace.cpp`, `constfold.cpp`, `reroll.cpp` | The graph passes, in pipeline order. Each has an env switch to turn it off (see OPTIMIZATIONS.md). |
| `runtime/src/executor.cpp` | Runs the op list: forward for the log density, reverse for the gradient. `STANLI_PROFILE=1` prints per-opcode accounting. |
| `runtime/kernels/` | Op implementations. `densities.cpp` instantiates unmodified stan-math prim templates; `elementwise.cpp`/`eltwise_expr.cpp` the vector math; `constrain.cpp` the transforms; `matrix_fns.cpp` and `legacy_fns.cpp` wrap stan-math functions that have no native port (see `legacy.hpp` for the mechanism). |
| `runtime/include/stanli/mir_interp.hpp` | The one MIR interpreter, templated on the scalar. Three users: the lowering (transformed data and every data-only expression, on `double`), the ODE kernels (right-hand sides the compiled path cannot handle, on `double` and `var`), and the interpreted write_array. |
| `runtime/src/wa_interp.cpp` | `WaInterp`: per-draw interpreted generated quantities for models whose write_array graph cannot be built (RNG calls, draw-dependent branches). Owns the RNG stream and the FnReadParam/FnWriteParam statement hooks. |
| `runtime/src/nuts.cpp` | The sampler: stan's own `adapt_diag_e_nuts` driven through `model_adapter.hpp`. |
| `runtime/src/capi.cpp`, `capi.h` | The C ABI the shared library exports. `python/stanli/__init__.py` is a thin ctypes wrapper over it. |
| `runtime/src/stanc_embed_c.cpp`, `tools/stanc_embed/` | The in-process stanc3: the OCaml compiler built with `-output-complete-obj` and linked into the shared library. |
| `tools/` | `stanli_check` (one deterministic gradient evaluation, machine-readable), `stanli_run` (full CSV sampling run), `dump_ops` (print a model's lowered op list), `verify_refs.py` (corpus replay against recorded CmdStan values, runs in CI), `verify_sample.py` (records those references, needs CmdStan), `sampler_trace.py` (sampler-column diff vs CmdStan), `gen_docs.py` (stamps measured numbers into the READMEs). |
| `harnesses/` | Corpus sweeps that need a local posteriordb: `wa_coverage.py` (how much of each model's generated quantities we produce), `wa_header_check.py` (CSV headers vs CmdStan), benchmarks. |
| `tests/` | One `test_*.cpp` per subsystem, plus `fixtures/` with `.stan` sources and their pinned `.tmir.sexp` MIR (regenerate with `tools/gen_fixtures.sh`). |

## Adding a stan-math function

Decide where it runs. Anything the log density touches needs to be an op;
functions that only appear in transformed data, ODE right-hand sides, or
generated quantities only need the interpreter.

Interpreter vocabulary: add a branch in `mir_interp.hpp`'s `eval_fun`
(deterministic math, templated on the scalar) or, for RNG draws, in
`wa_interp.cpp`'s `rng_fun`. Both fail loudly on anything unhandled, so
the corpus tells you what is missing:
`python3 harnesses/wa_coverage.py deps/posteriordb`.

An op takes four steps:

1. Opcode in `optable.hpp`, kernel in `runtime/kernels/`. For a stan-math
   function without a native port, wrap it with the mechanism in
   `legacy.hpp` (examples all over `matrix_fns.cpp`); it is correct by
   construction and can be replaced by a native kernel when it shows up
   in profiles.
2. Lowering branch in the matching `lower_*_fn` group in `lower.cpp`.
3. A test in the matching `tests/test_*.cpp`, asserting parity against
   the same stan-math call on `var` (house pattern in `test_lower.cpp`).
4. `build-rel/dump_ops model.stan data.json` shows what actually lowered.

## Verifying a change

```
cmake --build build-rel -j8 && (cd build-rel && ctest)
python3 tools/verify_refs.py deps/posteriordb --check build-rel/stanli_check --jobs 8
```

The second line replays all 119 corpus models against recorded CmdStan
values and is the strongest oracle in the project; it also runs in CI on
every push, on all four platforms. Compiler changes that claim to be pure
refactors should leave its worst-deviation line untouched. For sampler
changes use `tools/sampler_trace.py`; for generated-quantities coverage,
`harnesses/wa_coverage.py`; for performance claims, measure with
`STANLI_PROFILE=1` before and after (`docs/benchmarks.md` has the
harnesses and the current numbers).

Release process and CI layout live in `README.md` under "Releasing".
