# Graph optimizations and performance work

This file describes the optimizations stanli applies to a model before
running it, and a few performance details of the runtime itself. It is
written for people who have not worked on compilers. Each section says
what the problem is, what we do about it, and how to turn the fix off
when debugging.

## Background: what the graph is and why op count matters

stanli turns a Stan model into a list of operations ("ops"). Each op
reads some buffers, computes something, and writes a buffer. Evaluating
the log density means running the list top to bottom. Computing the
gradient means running it again bottom to top.

Running one op has a fixed cost of roughly 15-20 nanoseconds, no matter
how small the computation inside it is. An op that adds two single
numbers pays the same fixed cost as an op that adds two vectors of
10,000 numbers. So the main way to make a model faster is to make the
list shorter: replace many small ops with few big ones.

The problem is that lowering (the step that builds the list from the
compiler's output) unrolls loops. A model with

```stan
for (n in 1:1000) {
  y[n] ~ normal(mu[county[n]], sigma);
}
```

becomes several thousand single-number ops: one read of `mu`, one
density evaluation, and so on, a thousand times over. The optimizations
below exist to undo that, without changing the numbers the model
computes.

The passes run in this order, in `lower.cpp`:

1. in-place updates (`inplace.cpp`)
2. store-to-load forwarding (`inplace.cpp`)
3. constant folding (`constfold.cpp`)
4. loop re-rolling (`reroll.cpp`)

## In-place updates (`inplace.cpp`, disable: `STANLI_NO_INPLACE=1`)

Writing one element of a vector, like `mu[n] = x;` inside a loop, is
lowered as a "functional update": copy the whole vector into a new
buffer, then change one element. This keeps every intermediate version
of the vector around, which the gradient step sometimes needs. But it
means N writes into a vector of length N copy about N*N/2 numbers in
total. One real model (radon_county_intercept, N = 12,573) spent 90
milliseconds per gradient and 2.6 GB of memory this way.

The fix: if nothing ever looks at the old version of the vector again,
skip the copy and change the element directly in the existing buffer.

"Nothing looks at it again" has two parts:

- No later op reads the old version. The write must be the last use of
  that buffer.
- No earlier op needs the old values during the gradient step. Some
  ops (for example `log_sum_exp` before it got its own kernel)
  recompute their derivative from their input buffer when the gradient
  runs. The gradient runs after all the writes have happened, so if we
  destroyed the buffer, those ops would read wrong numbers. Only ops
  whose gradient step just moves numbers around, without re-reading
  input values, are safe to have before an in-place write. There is an
  explicit list of those ops (`backward_ignores_input_values`), and a
  test that checks every op on the list really has that property.

We found the need for the second condition the hard way: an earlier
version of this pass got eight models silently wrong by large amounts,
with nothing visibly different about the graphs. The full-corpus
comparison described at the end of this file is what caught it.

## Store-to-load forwarding (`inplace.cpp`, same switch)

A very common pattern is: write `mu[n]`, then read `mu[n]` back in the
same loop iteration. After lowering, that is a write op immediately
followed by a read op on the same element. The pass deletes the read
and hands the value straight to whoever wanted it. If, after this,
nobody reads the vector at all anymore, the writes are deleted too.

This matters because it turns "loop that fills a vector" into "loop
that just does arithmetic", which the re-rolling pass below knows how
to vectorize.

## Constant folding (`constfold.cpp`, disable: `STANLI_NO_CONSTFOLD=1`)

Some parts of a model do not depend on the parameters at all. For
example, `dogs` builds two 25x30 matrices purely out of its data in the
`transformed parameters` block. Those ops compute the same numbers on
every single gradient evaluation, thousands of times during sampling.

The pass finds every op that no parameter can influence, runs those ops
once, saves the resulting numbers, and deletes the ops. The saved
numbers are loaded into the buffers when the model is set up, the same
way data is.

To run the ops "once", the pass builds a small temporary graph out of
just the constant ops and runs it through the normal executor. This is
deliberate: the kernel that implements an op is the only definition of
what that op computes. If this pass had its own evaluation code, the
two could disagree.

One ordering rule: folding replaces a buffer with its final contents.
If some surviving op reads a buffer at a moment when it held earlier,
different contents (this happens with the in-place write chains, which
reuse one buffer), that buffer cannot be folded, and anything computed
from it cannot be folded either.

## Loop re-rolling (`reroll.cpp`, disable: `STANLI_NO_REROLL=1`)

This is the main pass. An unrolled loop shows up in the op list as the
same short pattern repeated many times: for example
"read element, multiply, add, evaluate density" over and over, once per
data point. The pass looks for such repeats. Each repetition is called
a lane. If it can prove the lanes are independent of each other, it
replaces all of them with a handful of vector ops.

To do that, it has to work out, for each input of the pattern, how that
input varies from lane to lane:

- Same buffer every lane: keep it as is. The vector kernels accept a
  single number where a vector is expected and reuse it for every
  element ("broadcasting").
- A different small constant every lane: collect the constants into a
  new vector, load it at setup time, use that.
- The output of an earlier op in the same lane: use the vectorized
  version of that op's output.

Reads and writes of vector elements get special handling:

- Reading `v[0], v[1], v[2], ...` across lanes, one element per lane,
  in order, covering the whole vector: no op needed at all. The
  consumers can just read `v` directly.
- Reading the same element every lane: keep one copy of the read
  ("hoisting" it out of the loop).
- Reading a contiguous range: one slice op.
- Reading arbitrary elements, like `mu[county[n]]`: one gather op that
  reads all the requested elements. Its gradient step adds each
  element's contribution back to the right place, including when the
  same element is read by many lanes.
- Writing `v[0], v[1], v[2], ...` across lanes: one store op that
  writes the whole range, or no op at all if the writes cover the whole
  vector (the computed vector simply becomes `v`). Writes that step by
  a fixed amount bigger than 1 (filling a matrix column by column) use
  a strided store op. When several write runs take turns filling one
  vector, each run's store output becomes the vector that everything
  afterwards refers to, so the runs chain together and each one can be
  vectorized on its own.
- A whole run of writes that all write the same value (this happens
  when every varying input of the value's computation turned out to be
  hoisted): the computation is kept as a single scalar and one
  broadcast op copies it across the range. Widening it blindly was a
  bug: the kernels would have read only the first element.

Densities get one more trick. If every lane's density result goes
straight into the target (the running log-density total), the N scalar
density ops become one vector density op. The vector kernels already
return the sum of the element densities, so this also removes N
additions from the target total. Discrete densities like
`bernoulli_logit` carry their integer outcome (the observed 0 or 1) as
a small attached integer rather than as a buffer; those fuse by
concatenating the attached integers of all lanes into one array, which
is exactly the form the vector kernel expects.

Anything the pass cannot prove safe, it leaves alone. The common
reasons to leave a region alone:

- One lane reads a result computed by the previous lane (a recurrence,
  like an AR model's state update, when it involves parameters).
- A lane's intermediate result is used outside the loop.
- The density results feed into another op (for example `log_mix`)
  instead of the target. This is the main remaining gap; mixture
  models still run as scalar loops because of it.
- An op the pass does not have a rule for.

When a repeated pattern only partly qualifies, the pass rewrites the
longest prefix of lanes that does qualify and tries again on the rest.
This is what handles data that comes in blocks (for example
observations sorted by time period): each block vectorizes separately.

Results, to give a sense of scale: `radon_pooled` drops from 27,670 ops
to 8, `radon_county` from 25,152 to 10, `election88_full` from 289,165
to 65, `dogs` from 12,751 to 261.

## Compiled ODE right-hand sides (`ode_prog.cpp`, report fallbacks: `STANLI_DEBUG_ODE=1`)

Models with differential equations pass a user-written function (the
"right-hand side") to a numerical solver. The solver decides at run
time which time points to evaluate it at, so this one function cannot
be unrolled into the graph ahead of time. It used to be executed by
walking the compiler's syntax tree every call, looking variables up by
name in a map and allocating a fresh vector for every intermediate
value. That cost about 6 microseconds per call, the solver makes
hundreds of calls per gradient, and it was 97% of the total time on
the ODE models.

Now the function is translated once, when the model is loaded, into a
simple instruction list: every variable becomes a numbered cell in one
flat array, every operation becomes an entry like "cell 14 = cell 3
times cell 9", and running the function is a single loop over the
entries. Loops inside the function with known bounds are unrolled at
translation time; conditions on run-time values become jump
instructions. No lookups, no memory allocation.

Anything the translator cannot handle falls back to the old
tree-walking interpreter, so no model loses support; it is just slower.
Setting `STANLI_DEBUG_ODE=1` prints when that happens. A test runs
every supported construct through both implementations and requires
identical results, bit for bit.

Separately, the gradient used to solve the differential equation twice
per evaluation, once for the values and once for the derivatives. The
first solve already computes everything needed for the derivatives (it
has to, for accuracy reasons), so we now save that information instead
of throwing it away, and the second solve is gone.

Together these made the ODE models 29-39x faster.

## Executor details (`executor.cpp`)

Two smaller things live in the executor rather than in a pass:

- Each op's "context" (the little bundle of pointers telling the kernel
  where its inputs, outputs, and workspace are) is built once when the
  model is set up, not rebuilt on every evaluation. All the pointers go
  into buffers that never move after setup, so there is nothing to
  rebuild. This mostly helps models with many small ops.
- All values live in one big array, all gradient values in another, and
  both are allocated once. A steady-state gradient evaluation performs
  no memory allocation at all.

## How we check all of this

Every optimization here changes the graph, and a wrong graph produces
wrong numbers silently. Three layers of checking:

- Unit tests per pass (`tests/test_reroll.cpp`, `tests/test_inplace.cpp`,
  `tests/test_ode_prog.cpp`, `tests/test_pass_safety.cpp`). These
  include tests that the passes refuse the cases they must refuse, and
  a fuzz test that runs hundreds of random graphs through all passes
  and compares gradients before and after.
- A full-corpus A/B check (`spikes/ab_corpus.py`). It evaluates every
  model in the posteriordb test set twice, once with the passes turned
  off and once with them on, and compares the log density and every
  gradient component. The passes-off graph is separately verified
  against CmdStan, so if A and B agree, the optimized graph inherits
  that verification. This check has caught two real bugs that the unit
  tests missed, on model shapes nobody thought to write a unit test
  for. Run it after any change to a pass.
- Direct verification against CmdStan (`tools/verify_sample.py`) for
  models whose graphs a change affects.

The environment variables (`STANLI_NO_INPLACE`, `STANLI_NO_CONSTFOLD`,
`STANLI_NO_REROLL`) exist so that a wrong result can be attributed to a
single pass quickly: turn them off one at a time and see which one
changes the answer.
