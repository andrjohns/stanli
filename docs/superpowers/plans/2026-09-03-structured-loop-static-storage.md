# Structured loops: one executor, static storage classes

Date: 2026-09-03. Branch `feat/loop-jit-prep`. Base: main after #312.

## Why

The retained-loop executor that landed in #306/#309/#311 stores a fresh
version of every kernel output, including whole-container copies for
`x[i] = v` and every data-only intermediate. Four runtime "plans" and a
four-band double-encoded handle scheme were then added to reclaim the memory
that model wastes. Measured on main (2026-09-03):

- `x[i] = f(x[i-1])` over a `vector[N]`: history is O(N^2). N=10k gradient is
  326x slower than unrolling and takes 1.6 GB; radon_county selects a 2.5 GB
  rectangle per Executor. With dynamic history the reverse pass is still
  O(N) per scalar update, so N=100k takes 5.7 s per gradient.
- Fixed dispatch: ~35 ns per kernel per gradient against ~15 ns flat. 38% of
  the time is rebuilding `KernelCtx` per call; the flat executor prebinds it.
- Every plan is a runtime decision. A stencil JIT of the body needs each
  node's storage decided at prepare time.

This change deletes the rectangular executor, the dynamic executor's plans
and handle bands, the target-fragment machinery, and the while/for capacity
proofs, and replaces them with one executor over static per-node storage
classes. The compiler side (`lower_structured_loop.inc`) keeps its body
lowering; only capacity requirements and the target export change.

## Payload

```cpp
struct StructuredLoop {
  struct Node {
    enum Kind { Sequence, KernelCall, Alias, If, For, While, Break, Continue,
                Target } kind;
    // KernelCall only, decided by prepare():
    enum Storage { Retained, Transient, InPlace } storage = Retained;
    bool active = false;        // static: this call's backward may run
    int invariant_loop = -1;    // dense loop index whose entry invalidates
                                // a cached result; -1 recomputes every visit
    uint32_t site = ~0u;        // dense KernelCall index
    int64_t workspace = -1;     // Transient: offset into the state workspace
    int64_t kernel_scratch = 0;
    int loop_index = -1;        // For/While: dense loop index
    std::vector<Node> children;
    int op = -1, dst = -1, src = -1;
    int condition = -1, iterator = -1, lower = -1, upper = -1;
    void (*forward)(KernelCtx&) = nullptr;
    void (*backward)(KernelCtx&) = nullptr;
  };
  struct Import { int slot = -1; int input = -1; int64_t offset = 0;
                  bool active = false; };
  Graph body;
  std::vector<std::pair<int, std::vector<double>>> fills;
  std::vector<Import> imports;
  std::vector<int> outputs;
  bool has_target = false;      // one scalar output after `outputs`
  Node root;
  int64_t initial_size = 0;     // sum of body slot lengths
  int64_t workspace_size = 0;   // sum of Transient out/out2/scratch lengths
  size_t node_count = 0, site_count = 0, loop_count = 0;
  void prepare();
};
```

Removed: `capacity`, `frame_size`, `target_capacity`, `compact_update_cell`,
`record_site`, `dynamic_history`, `target_fragment`, every `*_offset`,
`primal_size`, `scratch_size`, `record_node_count`, `compact_update_sites`,
`TargetReduction`, `OP_TARGET_REDUCE`, `prepare(int64_t max_bytes)`.

`Import.active` comes from the parent `Val::autodiff` of the imported value
(runtime dimension slots are inactive).

## prepare()

Validation as today (slot ranges, arity, kernel registered, no stateful
kernel, no in-place opcodes, no OP_ISLAND/OP_LOOP, Alias lengths equal,
Break/Continue inside a loop, control slots scalar). Then, in order:

1. Number sites (KernelCall preorder) and loops (For/While preorder). Assign
   `body.slots[i].offset` cumulatively; `initial_size` is the total.
2. Fuse in-place updates. In every Sequence, a `KernelCall` whose op is
   `OP_SET_INDEX_DYNAMIC` with `out = o`, `in[0] = B`, immediately followed by
   `Alias{dst = B, src = o}`, where `o` has no other reader anywhere in the
   tree (no other KernelCall input, no other Alias src, not a Target src, not
   in `outputs`), becomes `storage = InPlace` and the Alias is erased from the
   Sequence. For invariance and activity below, an InPlace node writes `B`.
3. Static activity. `active[slot]` starts true for active imports, false
   otherwise. Iterate to a fixed point over the whole tree: for a KernelCall
   with `backward != nullptr`, `active[out] |= any(active[in[k]])`, same for
   `out2`; for InPlace, `active[B] |= active[rhs]`; for Alias,
   `active[dst] |= active[src]`. A KernelCall is `active` iff it has a
   backward and any input is active (InPlace: base or rhs active).
4. Written sets. For each loop node L, `written(L)` = out/out2 of KernelCalls
   under L, dst of Aliases under L, B of InPlace nodes under L, iterators of
   For nodes under L (including L itself).
5. Invariance. For a KernelCall K (not InPlace, not `is_effectful_op`), walk
   its enclosing loops from outermost to innermost; `invariant_loop` is the
   outermost loop L such that no input slot of K is in `written(L)`.
   Otherwise -1. Nodes outside any loop stay -1.
6. Transient. A KernelCall K with `!active` is Transient iff for its `out`
   (and `out2`): no Alias has it as src, no Target has it as src, it is not
   in `outputs`, no InPlace node has it as base, and every KernelCall reading
   it is `!active`. Control reads (If/While condition, For bounds) are
   allowed. Transient nodes get `workspace` offsets covering out, out2 and
   kernel scratch. Everything else is Retained.

Out slots are unique per KernelCall (lowering emits a fresh slot per
`emit_value`); prepare asserts this.

## Executor state (one per bound Executor, `KernelState`)

```cpp
struct Version { double* value; int64_t adjoint; };
// adjoint >= 0: offset into `adjoints`; -1: inactive;
// <= -2: import ordinal -(adjoint + 2), adjoint lives in outer in_adj.
struct Record {
  uint32_t site; enum { Kernel, InPlace, Copy } kind;
  int64_t handles;                 // Kernel: offset into `handles`, n_in entries
  int64_t out, out2;               // Kernel: output versions (out2 = -1 if none)
  double* scratch;                 // Kernel
  int64_t base, rhs, undo, count;  // InPlace: versions and undo span
  int64_t from, to;                // Copy
};
struct LoopState : KernelState {
  BlockArena arena;                 // reached values and kernel scratch
  std::vector<double> workspace;    // Transient cells, sized once
  std::vector<Version> versions;
  std::vector<int64_t> bindings;    // slot -> version
  std::vector<int64_t> handles;     // saved input versions of Kernel records
  std::vector<double> undo;         // InPlace: count pairs (position, old)
  std::vector<Record> records;
  std::vector<int64_t> target_refs;
  std::vector<int32_t> owner;       // version -> slot that may mutate it, or -1
  std::vector<int64_t> node_generation;   // site -> loop generation when cached
  std::vector<int64_t> node_version, node_version2;  // cached outputs
  std::vector<int64_t> loop_generation;   // loop -> entries so far
  std::vector<KernelCtx> ctx;       // site -> prebound template
  std::vector<const Node*> sites;
  std::vector<double> adjoints; int64_t adjoint_size = 0;
  bool reverse_ready = false;
};
```

`BlockArena` is today's `DynamicArena` minus location handles: geometric
blocks, bump allocation, `clear()` keeps the first block.

`ctx[site]` is filled once at state construction with `n_in`, `variant`,
`idata`, `n_idata`, `udata`, `in[k].len`, `out.len`, `out2.len`, and
`in_adj[k].len`. Forward and backward only store pointers into it.

## Forward

1. Release the previous tape: clear arena, versions, handles, undo, records,
   target_refs, owner; `adjoint_size = 0`; `reverse_ready = false`.
2. Allocate the initial block (`initial_size`), copy fills and imports into it
   (imports are snapshotted as today), and create one inactive version per
   slot pointing into it. Import versions carry the import marker when the
   outer `in_adj` is non-null. `bindings[slot]` = that version.
3. Create the Transient versions (pointing into `workspace`) and store their
   indices in `node_version[site]`; reset `node_generation` to -1 and
   `loop_generation` to 0.
4. Walk the tree:
   - Sequence: children in order, propagate Break/Continue.
   - If: `value(condition) != 0 ? child 0 : child 1`.
   - For: validate integer bounds as today; `++loop_generation[loop]`; per
     iteration allocate one arena double for the iterator, create an inactive
     version, bind it, run the body; Break exits, Continue proceeds.
   - While: `++loop_generation[loop]`; loop { children[0]; if
     `value(condition) == 0` break; children[1] (Break exits) }. No capacity.
   - Break/Continue: return the flow.
   - Target: `target_refs.push_back(bindings[src])`.
   - Alias: `bindings[dst] = bindings[src]; owner[bindings[src]] = -1`.
   - KernelCall, `invariant_loop >= 0` and `node_generation[site] ==
     loop_generation[invariant_loop]`: rebind `out`/`out2` to the cached
     versions and return.
   - KernelCall Transient: bind ctx input pointers from `bindings`, out/out2/
     scratch into the workspace, call forward, bind outputs to the node's
     fixed versions.
   - KernelCall Retained: append `n_in` handles; allocate `out.len + out2.len
     + kernel_scratch` from the arena; bind pointers; call forward; create
     output versions (adjoint: if `active`, reserve `len` from `adjoint_size`,
     else -1); if `active`, push a Kernel record.
   - KernelCall InPlace: `V = bindings[B]`. If `owner[V] != B`: allocate a
     copy V' of V (same length) with `owner[V'] = B`, adjoint reserved if V is
     active or rhs is active, push a Copy record {V, V'}, rebind B to V'. Then
     if the version is inactive and rhs is active, reserve an adjoint for it
     (promotion). Compute the selected positions from the selector inputs
     with the existing `IndexRuntime` (factor the position walk out of
     `set_index_forward`), append `(position, old value)` pairs to `undo` in
     write order, write the rhs elements, push an InPlace record. `bindings[B]`
     is unchanged.
   - After any KernelCall that has `invariant_loop >= 0`, record
     `node_generation[site] = loop_generation[invariant_loop]` and the output
     versions in `node_version`/`node_version2`.
5. Copy `bindings[out]` values for `outputs` into `ctx.out`. If `has_target`,
   reduce the target leaf values with the six-way grouping used today
   (`dynamic_loop_forward`'s non-fragment branch) into the last output.
6. `reverse_ready = true`.

A kernel that throws propagates unchanged; the next forward releases the
partial tape. Reentrancy is not supported (nested OP_LOOP is refused at
prepare).

## Backward

1. Require `reverse_ready`, clear it. `adjoints.assign(adjoint_size, 0)`.
2. Seed: for each output slot, `adj(bindings[slot])[i] += out_adj_vec[pos+i]`
   when the version has an adjoint; for the target, every leaf in
   `target_refs` gets `+= out_adj_vec[last]`.
3. For records in reverse:
   - Kernel: bind ctx from the saved handles (value pointers, adjoint
     pointers or null), out/out2 values and adjoints (`out_adj` scalar when
     `out.len == 1`, `out2_adj`), scratch; call backward.
   - InPlace: for k from count-1 down to 0: `if (adj_base) { if (adj_rhs)
     adj_rhs[k] += adj_base[pos_k]; adj_base[pos_k] = 0; }
     base.value[pos_k] = old_k`.
   - Copy: `if (adj_from && adj_to) adj_from[i] += adj_to[i]`.
4. Release the tape.

`adj(version)`: `adjoint >= 0` → `adjoints.data() + adjoint`; `-1` → null;
import → `outer.in_adj[imports[k].input].data + imports[k].offset` (null when
the outer adjoint is null).

## Compiler changes (`lower_structured_loop.inc`, `lower.cpp`)

- `While` no longer requires `region_while_capacity`. Keep the counter range
  inference it performs when the proof succeeds; when it fails, lower the
  loop anyway with no range for the counter. Delete
  `positive_when_guard_reached` (the `x / ceil(x / scale)` idiom). If
  `real_ranges` / `region_real_range` / `region_enumerated_data_range` /
  `region_real_definitions` have no remaining consumer after that, delete
  them too.
- `For` inside a region no longer fails when `region_range` of a bound is
  unknown; the zero-trip and one-trip special cases apply only when both
  ranges are known. `loop.capacity` is gone.
- `try_lower_region`: `Import.active` from the parent `Val::autodiff`; the
  region output is `outputs` then one target scalar; the target term is an
  `OP_INDEX` of that scalar pushed to `target_terms`. Delete
  `target_fragments`, `reduce_target_sources`, `structured_history_used`,
  `STANLI_STRUCTURED_HISTORY_BYTES`, `region_physical_memory_bytes`, the
  `dynamic_history` selection, and the `scratch_bytes` diagnostic field
  (print `nodes=... kernels=...` only).
- `lower.cpp::run`: drop the fragmented-target branch.
- `optable.hpp`: remove `OP_TARGET_REDUCE`.

The selector (`region_auto_profitable`, `region_cost`, `StructuredMode`) is
unchanged in this step.

## Tests

`tests/test_structured_loop.cpp` replaces `test_structured_loop_production.cpp`.
Port every test that asserts semantics: values and gradients against the
unrolled path or a hand computation, refusal and rollback, selection under
auto/prefer/force, direct-index descriptors, zero/one/many trips, nested
for/while/if, break/continue, alias and copy-on-write behaviour, failure
then retry, two executors sharing a graph, effects. Drop tests that assert
plan internals, environment ablation switches, memory-profile output,
`scratch_bytes`, handle bands, or rectangular sizing. Add programmatic-plan
tests for: Transient classification (a compare feeding an If is Transient;
a compare feeding an Alias is not), invariant reuse for an active kernel
(its backward runs once and the gradient matches recomputation), InPlace on
an import base (copy then mutate; parent value untouched), InPlace with an
inactive base and active rhs (promotion), duplicate positions in one update
(last write wins, earlier rhs adjoints are zero), and a container read by an
active kernel before an in-place update (LIFO undo restores the value seen
by that kernel's backward).

Retained-vs-unrolled comparisons of `lp` and gradients use a relative
tolerance of 1e-12: the region sums its target leaves with its own six-way
tree, so bitwise parity with the unrolled reduction is not expected.

## Addendum: memoizing data-only subtrees across evaluations

Measured after the rewrite on ctsem N=33: 11.1M kernel calls per gradient, of
which 10.8M are inactive and 9.7M Transient. The active work is 210K records
and 127K in-place updates. Almost all of the inactive work is data-only
bookkeeping (`for (ri in 1:size(ms))` scans with a `while (whenyes == 0 ...)`
inside, row-boundary searches) whose values and control are identical in every
evaluation because they depend on data only. The unrolled path folds all of
it at compile time; the retained loop re-executes it per gradient, and the
one version per loop iteration and per persistent inactive value is where the
432 MB (vs 86 MB before the rewrite) went.

Fix: memoize whole data-only subtrees by visit ordinal, persisting across
evaluations in the executor state.

### Static analysis (prepare)

- `Import.data_only` comes from the parent `Val::si.param_free`.
- `param_dep[slot]` fixed point over the tree, independent of `active`:
  imports start at `!data_only`; every KernelCall (backward or not) makes
  `out`/`out2` param_dep if any input is; Alias propagates src to dst; InPlace
  makes the base param_dep if the rhs or any selector is. Control dependence:
  a write (kernel out/out2, alias dst, in-place base, for iterator) executed
  under any enclosing If/While whose condition slot is param_dep, or For whose
  bound slots are param_dep, makes the written slot param_dep.
- A node is `memo` when it is a maximal such node: it is a For, While, If, or
  a Sequence created by grouping a maximal run of consecutive KernelCall/Alias
  children of one Sequence, and inside it every KernelCall has all inputs
  data-only and is not `is_effectful_op`, there is no InPlace, no Target, no
  Break/Continue that would leave the node (a Break/Continue whose target loop
  is inside the node is fine), every If/While condition and For bound is
  data-only, and every enclosing If/While/For up to the root is data-only
  controlled. Do not mark a child of a memo node.
- `memo_outs`: the slots written inside the node (kernel out/out2, alias dst,
  for iterator) that are read anywhere outside it (kernel input, alias src,
  control, target, output). `memo_index` numbers memo nodes densely;
  `memo_count` goes on the payload.

Group runs before classification so the tree seen by the executor has the
grouping Sequences. A run of length one is still grouped (a lone data-only
kernel whose result feeds a parameter-dependent branch is worth skipping).

### Executor

`LoopState` gains `std::vector<std::vector<double>> memo_tape` (one per memo
node, entries of `memo_outs` total length, persistent across evaluations),
`std::vector<int64_t> memo_ordinal` (reset per forward), and `bool
memo_ready` (false until one forward completes; never cleared afterwards, a
failed later forward does not invalidate data-only values). `release()` does
not touch the tapes.

Forward, on reaching a memo node m: `k = memo_ordinal[m]++`.
- If `memo_ready`: require `k < entries(m)` else throw
  `logic_error("structured memo trace mismatch")`; for each live-out slot
  `bindings[slot] = make_version(tape[m].data() + k * total + offset, -1)`;
  return Normal without visiting children.
- Else: execute the node normally; afterwards append the live-out values
  (`value(slot)`, `len` each) to `tape[m]`. Pointers into `tape[m]` are only
  handed out once `memo_ready`, so growth during the first forward is safe.

Set `memo_ready = true` at the end of a successful forward. Memoized nodes
create no records, so backward is unchanged.

### Expected effect

ctsem N=33: the `while (whenyes ...)` loop (4.8M calls) and the per-row
condition cones (5 kernels x 700K visits) become one ordinal load and one
version per visit. Kernel calls should drop from 11.1M to well under 2M and
RSS back below the pre-rewrite 86 MB.

## Addendum 2: replaying data-only control decisions

After subtree memoization ctsem N=33 still performs 8M memo restores per
gradient (66 MB of tape). They come from loops whose body is a data-only guard
followed by an active arm, `for (ri in 1:size(ms)) if (m == ms[ri,7] && ...)
{ ... }`: the guard cone is memoized, but its result is a live-out because the
`If` reads it, so every visit restores a value and creates or moves a version.
The unrolled path keeps only the iterations whose guard is true.

The decision itself is data-only, so record it instead of the value, and
record which iterations of a data-only loop do anything at all.

### Static

In `group()`, when a node is reached with `ok == true` (every enclosing
If/While/For is data-only controlled) and is not memoizable as a whole:

- An `If` with `!controlled(n)` gets `trace = true`.
- A `For` with `!controlled(n)` whose body contains no parameter-dependent
  control (every If/While/For under it is `!controlled`) gets `trace = true`.
  The restriction keeps the set of effective iterations independent of the
  parameters.
- A `While` with `!controlled(n)` whose condition child `children[0]` is
  memoizable, and whose slots written there have no consumer outside it other
  than the loop's own condition read, gets `trace = true`; replay skips
  `children[0]` entirely.

Reads of `n.condition` (or the For bounds) by a traced node are not outside
reads in `number()`, so a guard or bound cone whose only consumer is a traced
node has no live-outs. `group()` merges any run of consecutive memoizable
children, not just KernelCall/Alias, into one memo node: ctsem's `&&` guards
lower to chains of data-only `If`s that would otherwise each restore a
live-out for the next. `trace_count` on the payload counts traced nodes.

### Executor

`LoopState` holds one sequential control trace, `std::vector<int64_t> trace`
with a read position reset every forward, filled while `!memo_ready` and read
afterwards. Replay order equals recording order because every traced decision
sits under data-only control. A mismatch throws
`logic_error("structured control trace mismatch")`.

An effects counter `effects` is incremented by everything observable: a kernel
call that actually runs (Transient, Retained or InPlace), an Alias, a Target,
Break, Continue, and a memo node with live-outs whether recording or
restoring. Work inside a memo node without live-outs is not observable: its
run during recording leaves the counter unchanged, and replay skips it.

Recording:

- Traced `If`: append the arm taken.
- Traced `While`: append a placeholder at entry, run as usual counting body
  executions, patch the count in at exit.
- Traced `For`: append a placeholder; for every iteration remember the trace
  length and the effects counter, append the iterator value, run the body,
  and if the effects counter did not move truncate the trace back to the
  remembered length (dropping the iteration's own inner decisions), else count
  it effective. Patch the effective count in at exit. Break is an effect, so
  the breaking iteration is kept.

Replay:

- Traced `If`: read the arm and run that child without reading the condition.
- Traced `While`: read the count `c` and run `children[1]` `c` times without
  running `children[0]`; Break inside the body still exits.
- Traced `For`: read the effective count, then for each read the iterator
  value, bind the iterator (cell or fresh version as today) and run the body;
  the bounds are not read.
- Memo node with no live-outs: return `Normal` immediately without touching
  its ordinal.

### Measured effect (ctsem N=33)

Before: `memo_restores=8051046 memo_tape=8198097`, 88 ms per gradient, 298 MB.
After: `memo_restores=95841 memo_tape=176387 traces=495 trace=648813
visits=1988500`, 27 ms per gradient, 188 MB. Parity with main is bitwise at
two points and across six replayed points in one process.
