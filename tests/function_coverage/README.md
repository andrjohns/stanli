# Integrated function coverage

These ten small mother-style models cover 411 distinct names: supported Stan
functions, higher-order solver variants, and five operators. Five models are
generated from the smallest verified overload of each of 242 names in the
conformance baseline. The other five cover containers, data-only functions,
multivariate densities/GLMs, RNGs, and higher-order calls.

This is **function-name coverage, not complete overload or input coverage**.
`manifest.json` records each case and the supported boundaries. In particular,
GP coordinates are data, GLMs use supported design/outcome shapes, and
`map_rect` covers only the implemented empty-job case. `integrate_1d` and
`gaussian_dlm_obs_lpdf` are explicitly unsupported, not silently skipped.
Parameter declaration transforms remain in their dedicated tests; a transform
being supported in a declaration does not imply its named function is callable.

Every case contributes to the log density and/or a retained output column.
Nonuniform container projections expose element permutations. Data-only
results multiply a parameter as well as appearing in output; RNGs are compared
with the same seed and chain. CmdStan supplies all reference numbers. At three
fixed unconstrained points the gate compares the log density, every gradient,
the full column schema, and every output value. It rejects nonfinite probes,
missing output, compilation failures and shape differences. The numerical
gate is the corpus replay's `abs(a-b)/max(1,abs(a),abs(b)) <= 1e-9`.

CTest runs freshness, harness mutation tests, and numerical replay on each PR:

```sh
python3 tools/gen_function_models.py --check
python3 tests/test_function_models.py
python3 tools/check_function_models.py --build build-rel
```

The generator checks runtime dispatch literals and X-macro registrations
against its recipes and the pinned signature inventory. Adding a named runtime
function without a probe fails freshness; there is no automatic "unsupported"
escape. This source audit is intentionally conservative and is not proof that
every overload or execution path is implemented. The exhaustive nightly sweep
and focused boundary tests retain those separate responsibilities.

To change coverage, edit `recipes.py` or `higher_order.stan`, regenerate, and
record all ten models with the pinned CmdStan checkout:

```sh
python3 tools/gen_function_models.py
python3 tools/check_function_models.py --build build-rel --record --jobs 2
```

The reference artifact records compiler/dependency identities and exact model
source hashes. Recording is independent of stanli's answers: all CmdStan rows
are saved before replay, and a mismatch still fails. Never change references
to match a stanli regression. The generated sources and reference artifact
are committed so normal CI needs no CmdStan build or reference download.
