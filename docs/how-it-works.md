# How stanli works, and why an interpreter can outrun a compiler

stanli runs Stan models with no C++ compiler on your machine, and on many
models it evaluates gradients faster than the natively compiled model
CmdStan produces. Both halves of that sentence sound wrong. This document
explains why they are not, for readers who know Stan well and C++ a
little.

## What "compiling a model" actually does

When CmdStan builds your model, two things happen. First `stanc`
translates your Stan program into a C++ file. Second, a C++ compiler
spends several seconds (and, the first time, several minutes of library
building) turning that file into machine code.

It is worth looking at what is in that generated C++ file, because it is
less than you might think. Your model block

```stan
y ~ normal(mu + tau * theta_tilde, sigma);
```

becomes, in essence,

```cpp
lp += normal_lpdf(y, mu + tau * theta_tilde, sigma);
```

where `normal_lpdf`, the vectorized arithmetic, the constraint
transforms, and everything else your model touches are calls into
stan-math, a library that was written, tested, and could have been
compiled long before your model existed. The generated code contributes
no new math. It contributes an *arrangement*: which library operations to
call, in what order, wired to which variables.

That observation is the whole trick. A Stan model is a composition drawn
from a fixed vocabulary of operations: density functions, constraint
transforms, matrix algebra, elementwise math. The vocabulary is closed,
because it is exactly the Stan function library. New models do not add
operations; they arrange existing ones. And an arrangement does not need
a compiler. An arrangement is data.

## The op graph

So stanli ships every operation precompiled, once, inside one shared
library, and turns your model into a data structure: a flat list of
operations, each one an opcode plus indexes into a big preallocated
array of doubles. For the eight schools model block above, the list
looks roughly like

```
op 0: MUL          tau, theta_tilde   -> t0        (vector scale)
op 1: ADD          mu, t0             -> theta     (broadcast add)
op 2: NORMAL_LPDF  y, theta, sigma    -> target    (summed vector density)
```

Evaluating the log density means walking the list and calling each
opcode's precompiled function. There is no machine code anywhere that is
specific to your model, in the same way that a spreadsheet does not
recompile Excel when you type a new formula.

Two things make this more than a toy.

**The compiler is the real compiler.** The official `stanc` (an OCaml
program) is linked into the library and runs in-process. Your model is
parsed, typechecked, and optimized by the same code CmdStan uses, and
stanli consumes its output. The Stan language you get is the Stan
language, not a reimplementation of most of it.

**The kernels are the real kernels.** The precompiled operations are
stan-math, the same library CmdStan's generated code calls, compiled with
the same floating point settings. This is why stanli's answers agree with
CmdStan's to the last bit on 45 of the 120 posteriordb test models, and
to within 2.6e-12 relative on the worst one: it is mostly running the
same instructions on the same numbers, just dispatched differently.

Because nothing is compiled at model load, "compiling" a model takes
milliseconds. That alone changes how the tool feels: the time from
`model.stan` to a first draw is about twenty times shorter than
CmdStan's, essentially all of which is the C++ compiler stanli does not
run.

## Gradients without code generation

HMC needs the gradient of the log density, and Stan gets gradients from
reverse-mode automatic differentiation. Reverse mode has a simple shape:
run the computation forward, remember what you did, then walk the record
*backward* applying the chain rule to accumulate derivatives. The record
is traditionally called a tape.

In CmdStan, the tape is built dynamically. Every scalar operation on a
parameter, every add and multiply and log inside every density, creates a
small object on an arena recording what happened, with a virtual method
that knows how to push derivatives backward. Evaluate the gradient once
and the tape is built, walked backward, and torn down. Evaluate it a
thousand times per iteration (which HMC does; each leapfrog step needs a
fresh gradient) and this construction and teardown happens a thousand
times.

stanli does not build a tape, because it already has one. The op list
*is* the record of the computation: to differentiate, walk the same list
backward and call each opcode's derivative function, which each kernel
carries alongside its forward function. The tape is constructed once, at
model load, and steady-state gradient evaluation allocates no memory at
all. The entire autodiff engine in stanli is about 150 lines: a forward
loop, a backward loop, and an array of derivative accumulators.

## How an interpreter can win

Interpreters are supposed to be slow, and the intuition behind that is
right: there is a fixed overhead, some tens of nanoseconds, for every
operation dispatched. The question is what that overhead is amortized
over, and here the two designs pay their costs at different granularity.

**stanli pays per operation. CmdStan pays per scalar.**

When stanli executes `NORMAL_LPDF` over a thousand observations, it pays
its interpreter overhead once, then runs precompiled vectorized code over
a flat array of doubles. The overhead is real (roughly 20ns against 1ns
of arithmetic for a single scalar density) but divided by a thousand
elements it vanishes. This is the same reason vectorized R and NumPy are
fast: the interpreter only touches the outside of the loop.

CmdStan's generated code is native, so it has no dispatch overhead at
all, but its autodiff cost scales with scalars, not statements. Each of
the thousand observations allocates its tape node, and the backward pass
walks a thousand heap objects through virtual calls, every leapfrog step.
There is a second, subtler cost: CmdStan's autodiff matrices store
pointers to their tape nodes, so the actual values sit scattered in
memory. Reading them is indirect and strided, which defeats the SIMD
units modern CPUs rely on. stanli's values live in flat contiguous
arrays, because the graph, not the scalar type, carries the derivative
structure.

On a model written with vectorized statements, both effects point the
same way, and stanli evaluates gradients two to six times faster than the
compiled model, with the gap growing with data size. Not because it does
less math, but because it does less bookkeeping around the same math.

**The graph is a persistent artifact, and that means it can be
optimized.**

There is a deeper advantage hiding here. CmdStan's tape lives for one
gradient evaluation, so there is no opportunity to optimize it; it is
gone before anything could look at it. stanli's graph is built once and
reused millions of times, so it is worth running compiler-style passes
over it before the first evaluation.

The clearest example: models written as explicit loops,

```stan
for (n in 1:N)
  y[n] ~ normal(alpha[county[n]], sigma);
```

initially lower to N copies of a small handful of scalar operations,
and at scalar granularity the interpreter overhead really does hurt; early
versions of stanli lost to CmdStan on every model shaped like this. But
the N copies are sitting right there in a list, visibly periodic. A pass
detects the repetition and rewrites it: gathers become one vector index
operation, N scalar densities become one summed vector density. One
radon model drops from 27,670 operations to 8, and goes from slightly
losing against CmdStan to beating it six-fold. The statistician did not
vectorize the model; the runtime did, once, at load time, which is a
thing you can only do to a computation that exists as an inspectable
data structure.

## Where the compiled model still wins

Honesty requires the other column. One benchmark model
(`low_dim_gauss_mix`) still runs at about half CmdStan's speed: it
computes `log_mix` per observation, a shape the rewriting pass does not
yet know how to vectorize, so it stays as thousands of scalar operations.
ODE models run at 0.6x: the right-hand side of an ODE is the one place
Stan requires calling back into user code at times chosen by the solver,
and stanli evaluates it through a compact bytecode program where CmdStan
runs natively compiled code. Both gaps are engineering rather than
anything fundamental, but today they are real.

The other cost is size, and it is the deliberate trade at the center of
the design: the wheel is about 5 MB because it carries the entire Stan
compiler and every operation in the math library, whether your model
uses them or not. In exchange, nothing is ever built on your machine,
and `pip install stanli` is the entire installation.

## The one-paragraph version

Stan models are arrangements of a fixed library of operations, and an
arrangement is data, not code. Ship the library precompiled, turn the
model into a graph of operations over flat arrays, and the graph is
simultaneously the program, the autodiff tape, and an optimizable
artifact. Interpreting at vector granularity costs almost nothing; never
rebuilding the tape saves what CmdStan spends every leapfrog step; and
optimizing the persistent graph recovers the vectorization that loops in
the source hide. That is how there is no compiler, and why its absence is
sometimes a speedup.
