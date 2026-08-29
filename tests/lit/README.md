# Stan source lit tests

Every marked `.stan` file below `tests/` becomes its own CTest automatically.
New standalone cases live below this directory; existing fixture sources can
opt in where they already are. No CMake edit or C++ test target is needed.

Each file has two required directives and one optional data directive:

```stan
// STANLI-LIT: PASS
// STANLI-LIT-EXPECT: OK
// STANLI-LIT-DATA: {"N": 3}
```

- `STANLI-LIT` is `PASS` for supported behavior and `XFAIL` for a known gap.
- `STANLI-LIT-EXPECT` is `OK`, `CRASH`, or a substring of the expected
  `COMPILE_FAIL`/`EVAL_FAIL` result.
- `STANLI-LIT-DATA` is JSON. It defaults to `{}` when omitted.

A matching `XFAIL` passes CTest. Any changed result fails so the case is
reviewed and, when fixed, promoted to `PASS` with its new expectation. This is
also how a silently accepted invalid model is recorded: `XFAIL` plus `OK`.
XFAIL cases also carry the CTest label `broken`, so the known-gap inventory is
directly runnable with `ctest --test-dir build -L broken`.

The runner copies each source into a temporary directory before invoking the
pinned stanc, because stanc writes a sibling `.hpp` even when MIR is sent to
stdout. If the core developer setup has no stanc, the cases report a CTest
skip. Compiler-bearing CI runs all of them.

Run the complete source-lit suite with:

```sh
ctest --test-dir build --parallel 24 --output-on-failure -L lit
```
