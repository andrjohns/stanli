# Loop Re-Roll Pass Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** A post-lowering graph pass that detects unrolled scalar-loop regions (periodic op templates over consecutive lanes) and rewrites them into vectorized ops, flipping the scalar-loop models from 0.4-0.9x CmdStan to the measured 5.8-7.2x ceiling.

**Architecture:** The pass runs inside `Lowering::run()` after all statements lower but before `reduce_terms` builds the ADD_N tree, so N scalar density target terms can be swapped for one summed vector-density term. It operates on `Graph` + `CompiledModel::fills` + `target_terms` only (no Lowering internals), making it unit-testable on hand-built graphs. Detection is periodicity matching (validated against real graphs: period 2 radon_pooled, 16 arK, 7 low_dim_gauss_mix); inputs classify as lane-invariant / per-lane-const / lane-local / full-range INDEX progression; anything else bails per-region (never per-model).

**Tech Stack:** C++17, existing stanli kernels only (no new kernels in this plan). Vector shapes are runtime-dispatched by existing kernels (len==1 broadcasts), so widening a scalar op to a vector op keeps the same opcode.

## Global Constraints

- `-ffp-contract=off` project-wide; do not change compile flags.
- Re-rolled models will NOT be bitwise vs CmdStan (summation order changes); differential verification must stay within the corpus rig's tolerance. Spike-measured deviation: ≤4.1e-15 relative.
- `STANLI_NO_REROLL=1` env var must disable the pass entirely (A/B and bisection escape hatch).
- No commit message attribution footers.
- Baseline before this plan: 16/16 ctest green at main `1b629cf` + spike commit; corpus per docs: 119/120 evaluate, ~118 verified.
- Bench protocol (macOS has no core pinning): interleave A/B binaries within one script, ≥3 reps, medians, ratios not absolutes; spot-check load (`ps aux -r | head`) before timing; treat >10% rep spread as a rerun signal.

## Measured constants the pass is built on (from spikes/, 2026-08-05)

- Scalar density op all-in cost ~17-20 ns (9.5 executor + 9 recorder + 0.9 math); vector op amortizes this by N.
- Ceiling via hand-vectorized sources: radon_pooled 394→58µs, arK 30→1.7µs, gauss_mix 190→93µs (partial; needs batched LSE2 — out of scope here, phase 2).
- Real-graph structure (tools/dump_ops.cpp): lanes strictly consecutive; INDEX idata strides are 0,1,2,…; per-lane data values live in DEDUPED len-1 const slots (equal values share a slot — collect values, never assume distinct slots or arena adjacency).

---

### Task 1: Pass skeleton + radon-shape re-roll (INDEX elision, const materialization, density-to-target)

**Files:**
- Create: `runtime/include/stanli/reroll.hpp`
- Create: `runtime/src/reroll.cpp`
- Create: `tests/test_reroll.cpp`
- Modify: `CMakeLists.txt` (add `runtime/src/reroll.cpp` to BOTH `stanli_shared` and `stanli` source lists; add `stanli_add_test(test_reroll)`)

**Interfaces:**
- Produces: `RerollStats reroll(Graph& g, std::vector<std::pair<int, std::vector<double>>>& fills, std::vector<int>& target_terms)` in `namespace stanli`, declared in `reroll.hpp`. `RerollStats { int regions; int64_t ops_before, ops_after; }`. Later tasks call exactly this.

- [ ] **Step 1: Write the failing test**

`tests/test_reroll.cpp`, following the `test_executor.cpp` main()+counter style. Helper runs a graph pre/post pass and compares gradients:

```cpp
// Re-roll pass: unrolled scalar-loop regions collapse to vector ops with
// gradients preserved (up to summation order, 1e-12 rel).
#include <stanli/graph.hpp>
#include <stanli/optable.hpp>
#include <stanli/reroll.hpp>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static int failures = 0;
static void expect(const char* what, bool ok) {
  if (!ok) { ++failures; std::printf("FAIL %s\n", what); }
}
static void expect_close(const char* what, double got, double want) {
  const double rel = std::abs(got - want) /
                     std::max(std::abs(want), 1e-300);
  if (!(rel < 1e-12)) {
    ++failures;
    std::printf("FAIL %-24s got %.17g want %.17g rel %.2e\n", what, got,
                want, rel);
  }
}

using namespace stanli;
using Fills = std::vector<std::pair<int, std::vector<double>>>;

// Executes gradient at fixed params; returns {lp, grads...}.
static std::vector<double> run_grad(Graph g, const Fills& fills) {
  Executor ex(std::move(g));
  for (const auto& f : fills) {
    double* p = ex.value_ptr(f.first);
    for (size_t j = 0; j < f.second.size(); ++j) p[j] = f.second[j];
  }
  for (int64_t i = 0; i < ex.n_params(); ++i)
    ex.params_data()[i] = 0.2 + 0.1 * (i % 3);
  std::vector<double> out(1 + ex.n_params());
  out[0] = ex.gradient(out.data() + 1);
  return out;
}

// radon shape: mu = vector intermediate written by an op; per lane
// {INDEX(mu,n); NORMAL(y_const_n, idx, sigma)}, lp -> target term.
// y consts deliberately share slots (dedup pool) between lanes 1 and 5.
static void test_radon_shape() {
  const int L = 8;
  Graph g;
  Fills fills;
  const int alpha = g.add_slot(1, true);
  const int sigma = g.add_slot(1, true);
  const int base = g.add_slot(L, false);
  g.add_op(OP_REP_VEC, {alpha}, base);  // makes base a written slot
  std::vector<int> yconst(L);
  for (int n = 0; n < L; ++n) {
    if (n == 5) { yconst[n] = yconst[1]; continue; }  // dedup'd pool
    yconst[n] = g.add_slot(1, false);
    fills.emplace_back(yconst[n], std::vector<double>{0.25 * n - 1.0});
  }
  std::vector<int> terms;
  for (int n = 0; n < L; ++n) {
    const int idx = g.add_slot(1, false);
    g.add_op(OP_INDEX, {base}, idx, {n});
    const int lp = g.add_slot(1, false);
    const int id = g.add_op(OP_NORMAL_LPDF, {yconst[n], idx, sigma}, lp);
    g.ops[id].variant = 0x06;
    terms.push_back(lp);
  }
  // Reference BEFORE the pass (reduce terms manually via ADD_N pairs).
  Graph ref = g;
  {
    int acc = terms[0];
    for (int n = 1; n < L; ++n) {
      const int s = ref.add_slot(1, false);
      ref.add_op(OP_ADD_N, {acc, terms[n]}, s);
      acc = s;
    }
    ref.result_slot = acc;
  }
  const std::vector<double> want = run_grad(std::move(ref), fills);

  std::vector<int> tt = terms;
  Fills f2 = fills;
  RerollStats st = reroll(g, f2, tt);
  expect("radon regions==1", st.regions == 1);
  // 1 REP_VEC survives; 8 INDEX + 8 NORMAL collapse to 1 NORMAL.
  expect("radon ops==2", g.ops.size() == 2);
  expect("radon one term", tt.size() == 1);
  expect("radon vec y filled",
         f2.size() == fills.size() + 1);  // one materialized const vector
  g.result_slot = tt[0];
  const std::vector<double> got = run_grad(std::move(g), f2);
  expect("radon sizes", got.size() == want.size());
  for (size_t i = 0; i < want.size(); ++i)
    expect_close(("radon v" + std::to_string(i)).c_str(), got[i], want[i]);
}

int main() {
  test_radon_shape();
  if (failures) { std::printf("%d failures\n", failures); return 1; }
  std::printf("test_reroll OK\n");
  return 0;
}
```

Check while writing: confirm `OP_REP_VEC`'s actual input signature in `runtime/kernels/` (it may take a length via idata or slot len); adjust the base-writer op if needed — any op that writes a len-L slot from `alpha` works; the point is `base` must be a *written* slot so INDEX-elision substitutes a written base.

- [ ] **Step 2: Add CMake entries, run test to verify it fails**

Run: `cmake -B build-rel -DCMAKE_BUILD_TYPE=Release && cmake --build build-rel -j --target test_reroll`
Expected: link error `undefined symbol: stanli::reroll(...)` (or compile error: missing reroll.hpp).

- [ ] **Step 3: Implement the pass for this shape**

`runtime/include/stanli/reroll.hpp`:

```cpp
// Re-roll pass: rewrites unrolled-loop regions (periodic op templates over
// consecutive lanes) into vectorized ops. Runs after lowering, before the
// target-term reduction, so N scalar density terms can become one summed
// vector-density term.
#ifndef STANLI_REROLL_HPP
#define STANLI_REROLL_HPP

#include <stanli/graph.hpp>

#include <cstdint>
#include <utility>
#include <vector>

namespace stanli {

struct RerollStats {
  int regions = 0;
  int64_t ops_before = 0;
  int64_t ops_after = 0;
};

// In place. `fills` gains entries for materialized constant vectors.
// `target_terms` entries produced by vectorized densities are replaced by
// the single summed output slot (at the first lane's position). Slots are
// appended to g.slots; callers with arrays parallel to slots must resize.
RerollStats reroll(Graph& g,
                   std::vector<std::pair<int, std::vector<double>>>& fills,
                   std::vector<int>& target_terms);

}  // namespace stanli

#endif
```

`runtime/src/reroll.cpp` core (Task 1 scope — invariant inputs, per-lane consts, full-range INDEX elision, allowlisted densities with all lane outputs in target_terms):

```cpp
#include <stanli/optable.hpp>
#include <stanli/reroll.hpp>

#include <algorithm>
#include <cstdlib>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace stanli {
namespace {

constexpr int kMinLanes = 4;
constexpr int kMaxPeriod = 32;

bool is_density(uint16_t oc) {
  switch (oc) {
    case OP_NORMAL_LPDF: case OP_CAUCHY_LPDF: case OP_STUDENT_T_LPDF:
    case OP_GAMMA_LPDF: case OP_BETA_LPDF: case OP_LOGNORMAL_LPDF:
    case OP_UNIFORM_LPDF: case OP_DOUBLE_EXP_LPDF: case OP_EXPONENTIAL_LPDF:
    case OP_INV_GAMMA_LPDF: case OP_STD_NORMAL_LPDF:
      return true;   // real-arg lpdfs whose vector form returns the sum;
    default:         // verify each against densities.cpp when extending
      return false;
  }
}

struct Region { size_t start; int period; int lanes; };

// Ops match as template instances: idata may differ only for OP_INDEX.
bool ops_match(const Graph& g, const Op& a, const Op& b) {
  if (a.opcode != b.opcode || a.variant != b.variant ||
      a.n_in != b.n_in || a.out2 >= 0 || b.out2 >= 0)
    return false;
  for (int j = 0; j < a.n_in; ++j)
    if (g.slots[a.in[j]].len != g.slots[b.in[j]].len) return false;
  if (g.slots[a.out].len != g.slots[b.out].len) return false;
  if (a.opcode == OP_INDEX) return a.n_idata == 1 && b.n_idata == 1;
  if (a.n_idata != b.n_idata) return false;
  for (int64_t k = 0; k < a.n_idata; ++k)
    if (a.idata[k] != b.idata[k]) return false;
  return true;
}

struct Analysis {
  // Per template position p: kind of each input, producer position for
  // lane-local inputs, collected const values, hoisted/elided mapping.
  enum InKind { INVARIANT, CONST_LANES, LANE_LOCAL, BAD };
  struct PosIn { InKind kind = BAD; int producer_pos = -1;
                 std::vector<double> values; };
  struct Pos {
    std::vector<PosIn> ins;
    bool is_index_elision = false;  // full-range INDEX: out -> base slot
    bool all_invariant = false;     // hoist: emit lane 0's op once
    bool term_density = false;      // density, every lane's out is a term
  };
  std::vector<Pos> pos;
};

}  // namespace

RerollStats reroll(Graph& g,
                   std::vector<std::pair<int, std::vector<double>>>& fills,
                   std::vector<int>& target_terms) {
  RerollStats st;
  st.ops_before = (int64_t)g.ops.size();
  if (std::getenv("STANLI_NO_REROLL")) { st.ops_after = st.ops_before; return st; }

  // slot -> constant value (len-1 fills only; the dedup'd const pool).
  std::unordered_map<int, double> const_val;
  for (const auto& f : fills)
    if (f.second.size() == 1) const_val.emplace(f.first, f.second[0]);

  // slot -> consuming op indices; term membership; producer op index.
  const auto build_uses = [&] {
    std::unordered_map<int, std::vector<size_t>> uses;
    for (size_t i = 0; i < g.ops.size(); ++i)
      for (int j = 0; j < g.ops[i].n_in; ++j)
        uses[g.ops[i].in[j]].push_back(i);
    return uses;
  };
  std::unordered_set<int> term_set(target_terms.begin(), target_terms.end());

  std::vector<Op> result;
  size_t i = 0;
  while (i < g.ops.size()) {
    bool rewrote = false;
    for (int P = 1; P <= kMaxPeriod && i + 2 * P <= g.ops.size(); ++P) {
      // Count lanes.
      int L = 1;
      while (i + (size_t)(L + 1) * P <= g.ops.size()) {
        bool ok = true;
        for (int p = 0; p < P && ok; ++p)
          ok = ops_match(g, g.ops[i + p], g.ops[i + (size_t)L * P + p]);
        if (!ok) break;
        ++L;
      }
      if (L < kMinLanes) continue;

      // ---- classify ----
      auto uses = build_uses();
      Analysis an;
      an.pos.resize(P);
      // out slot of template position p in lane l:
      const auto out_of = [&](int p, int l) {
        return g.ops[i + (size_t)l * P + p].out;
      };
      std::unordered_map<int, int> lane0_producer;  // lane-0 out slot -> pos
      bool ok = true;
      for (int p = 0; p < P && ok; ++p) {
        const Op& t = g.ops[i + p];
        Analysis::Pos& ap = an.pos[p];
        ap.ins.resize(t.n_in);
        bool all_inv = true;
        for (int j = 0; j < t.n_in && ok; ++j) {
          bool invariant = true;
          for (int l = 1; l < L && invariant; ++l)
            invariant = g.ops[i + (size_t)l * P + p].in[j] == t.in[j];
          if (invariant) { ap.ins[j].kind = Analysis::INVARIANT; continue; }
          all_inv = false;
          // lane-local?
          auto pit = lane0_producer.find(t.in[j]);
          if (pit != lane0_producer.end()) {
            bool local = true;
            for (int l = 1; l < L && local; ++l)
              local = g.ops[i + (size_t)l * P + p].in[j] ==
                      out_of(pit->second, l);
            if (local) {
              ap.ins[j].kind = Analysis::LANE_LOCAL;
              ap.ins[j].producer_pos = pit->second;
              continue;
            }
          }
          // per-lane const?
          std::vector<double> vals(L);
          bool all_const = true;
          for (int l = 0; l < L && all_const; ++l) {
            auto cit = const_val.find(g.ops[i + (size_t)l * P + p].in[j]);
            if (cit == const_val.end()) all_const = false;
            else vals[l] = cit->second;
          }
          if (all_const) {
            ap.ins[j].kind = Analysis::CONST_LANES;
            ap.ins[j].values = std::move(vals);
            continue;
          }
          ok = false;  // unclassifiable input
        }
        if (!ok) break;
        // Position-level classification.
        if (t.opcode == OP_INDEX) {
          if (ap.ins[0].kind == Analysis::INVARIANT) {
            // idata progression check: full-range stride-1 elision, or
            // fully invariant idata (hoist).
            bool inv_idata = true, prog = true;
            for (int l = 0; l < L; ++l) {
              const int v = g.ops[i + (size_t)l * P + p].idata[0];
              if (v != t.idata[0]) inv_idata = false;
              if (v != l) prog = false;
            }
            if (prog && g.slots[t.in[0]].len == L) {
              ap.is_index_elision = true;
            } else if (inv_idata) {
              ap.all_invariant = true;
            } else {
              ok = false;  // partial/strided progression: Task 3 bails here
            }
          } else ok = false;
        } else if (all_inv) {
          ap.all_invariant = true;
        }
        if (!ok) break;
        // Register lane-0 producer for downstream lane-local matching.
        lane0_producer[t.out] = p;
        // Output discipline: every lane's out is consumed only inside its
        // own lane (later positions) or is a target term (densities only).
        const bool density = is_density(t.opcode);
        bool outs_are_terms = true, outs_internal = true;
        for (int l = 0; l < L; ++l) {
          const int o = out_of(p, l);
          if (!term_set.count(o)) outs_are_terms = false;
          for (size_t u : uses.count(o) ? uses[o].size() ? 0 : 0 : 0; false;) {}
          auto uit = uses.find(o);
          if (uit != uses.end())
            for (size_t u : uit->second) {
              const bool inside = u >= i + (size_t)l * P + p &&
                                  u < i + (size_t)(l + 1) * P;
              if (!inside) outs_internal = false;
            }
        }
        if (density && outs_are_terms) {
          ap.term_density = true;
          if (an.pos[p].is_index_elision) ok = false;  // impossible combo
        } else if (!outs_internal || term_set.count(t.out)) {
          ok = false;  // out escapes the lane and is not a clean term
        }
      }
      if (!ok) continue;
      // Require at least one term density (this is what makes the region
      // profitable and gives it a single external effect).
      bool any_term = false;
      for (auto& p : an.pos) any_term |= p.term_density;
      if (!any_term) continue;

      // ---- rewrite ----
      std::vector<int> pos_out(P, -1);  // vectorized out slot per position
      std::vector<Op> new_ops;
      for (int p = 0; p < P; ++p) {
        const Op& t = g.ops[i + p];
        Analysis::Pos& ap = an.pos[p];
        if (ap.is_index_elision) { pos_out[p] = t.in[0]; continue; }
        if (ap.all_invariant) {   // hoist lane 0's op verbatim
          new_ops.push_back(t);
          pos_out[p] = t.out;
          continue;
        }
        Op op = t;  // copies opcode, variant, idata
        for (int j = 0; j < t.n_in; ++j) {
          switch (ap.ins[j].kind) {
            case Analysis::INVARIANT: break;  // keep t.in[j]
            case Analysis::LANE_LOCAL:
              op.in[j] = pos_out[ap.ins[j].producer_pos];
              break;
            case Analysis::CONST_LANES: {
              const int cs = g.add_slot(L, false);
              fills.emplace_back(cs, ap.ins[j].values);
              op.in[j] = cs;
              break;
            }
            default: break;
          }
        }
        if (ap.term_density) {
          op.out = g.add_slot(1, false);  // summed lp
          // Replace L term entries with the one summed slot, at the first
          // lane's position in target_terms.
          bool placed = false;
          std::unordered_set<int> dead;
          for (int l = 0; l < L; ++l) dead.insert(out_of(p, l));
          std::vector<int> tt2;
          tt2.reserve(target_terms.size());
          for (int s : target_terms) {
            if (dead.count(s)) {
              if (!placed) { tt2.push_back(op.out); placed = true; }
            } else tt2.push_back(s);
          }
          target_terms = std::move(tt2);
          term_set.erase(dead.begin()->first, dead.begin()->first);  // rebuild below
          term_set = std::unordered_set<int>(target_terms.begin(),
                                             target_terms.end());
        } else {
          op.out = g.add_slot(L, false);
        }
        pos_out[p] = op.out;
        new_ops.push_back(op);
      }
      result.insert(result.end(), new_ops.begin(), new_ops.end());
      i += (size_t)P * L;
      ++st.regions;
      rewrote = true;
      break;
    }
    if (!rewrote) { result.push_back(g.ops[i]); ++i; }
  }
  g.ops = std::move(result);
  st.ops_after = (int64_t)g.ops.size();
  return st;
}

}  // namespace stanli
```

NOTE the sketch above contains two known rough spots to clean while implementing (they are marked by nonsense placeholders that will not compile): the stray `for (size_t u : uses.count(o) ...)` line must be deleted (the real loop is the three lines below it), and the `term_set.erase(dead.begin()...)` line must be deleted (the full rebuild on the next line is the real code). Implement `build_uses` ONCE per region attempt, not per position. Add `#include <cstdint>` if the compiler complains.

- [ ] **Step 4: Run test to verify it passes**

Run: `cmake --build build-rel -j --target test_reroll && ./build-rel/test_reroll` (from worktree root)
Expected: `test_reroll OK`

- [ ] **Step 5: Run the full suite, then commit**

Run: `ctest --test-dir build-rel` — expected 17/17 (16 existing + test_reroll).

```bash
git add runtime/include/stanli/reroll.hpp runtime/src/reroll.cpp tests/test_reroll.cpp CMakeLists.txt
git commit -m "feat: re-roll pass, radon shape (INDEX elision, const vectors, summed vector density)"
```

### Task 2: arK shape — invariant-op hoisting and lane-local elementwise widening

**Files:**
- Modify: `runtime/src/reroll.cpp` (widening allowlist)
- Modify: `tests/test_reroll.cpp` (add test)

**Interfaces:**
- Consumes: `reroll(...)` from Task 1.
- Produces: same signature; additionally rewrites regions whose templates contain `OP_ADD`/`OP_SUB`/`OP_MUL`/`OP_DIV` lane-local chains and all-invariant hoistable ops.

- [ ] **Step 1: Write the failing test** (append to `tests/test_reroll.cpp`, call from `main`)

```cpp
// arK shape: per lane {INDEX(beta,k) x2, MUL, ADD, ADD, NORMAL}.
// beta INDEX ops are lane-invariant (same idata) -> hoisted once.
// MUL second args are per-lane consts (the lag values).
static void test_ark_shape() {
  const int L = 6, K = 2;
  Graph g;
  Fills fills;
  const int alpha = g.add_slot(1, true);
  const int beta = g.add_slot(K, true);
  const int sigma = g.add_slot(1, true);
  std::vector<std::vector<int>> lag(K, std::vector<int>(L));
  std::vector<int> yobs(L);
  auto cslot = [&](double v) {
    const int s = g.add_slot(1, false);
    fills.emplace_back(s, std::vector<double>{v});
    return s;
  };
  for (int l = 0; l < L; ++l) {
    for (int k = 0; k < K; ++k) lag[k][l] = cslot(0.3 * l + 0.1 * k);
    yobs[l] = cslot(0.5 * l - 0.7);
  }
  std::vector<int> terms;
  for (int l = 0; l < L; ++l) {
    int mu = alpha;
    for (int k = 0; k < K; ++k) {
      const int bk = g.add_slot(1, false);
      g.add_op(OP_INDEX, {beta}, bk, {k});
      const int prod = g.add_slot(1, false);
      g.add_op(OP_MUL, {bk, lag[k][l]}, prod);
      const int acc = g.add_slot(1, false);
      g.add_op(OP_ADD, {mu, prod}, acc);
      mu = acc;
    }
    const int lp = g.add_slot(1, false);
    const int id = g.add_op(OP_NORMAL_LPDF, {yobs[l], mu, sigma}, lp);
    g.ops[id].variant = 0x86;  // propto + mu,sigma active, like real arK
    terms.push_back(lp);
  }
  Graph ref = g;
  {
    int acc = terms[0];
    for (int l = 1; l < L; ++l) {
      const int s = ref.add_slot(1, false);
      ref.add_op(OP_ADD_N, {acc, terms[l]}, s);
      acc = s;
    }
    ref.result_slot = acc;
  }
  const std::vector<double> want = run_grad(std::move(ref), fills);

  std::vector<int> tt = terms;
  Fills f2 = fills;
  RerollStats st = reroll(g, f2, tt);
  expect("ark regions==1", st.regions == 1);
  // 2 hoisted INDEX + 2 vec MUL + 2 vec ADD + 1 vec NORMAL = 7 ops.
  expect("ark ops==7", g.ops.size() == 7);
  expect("ark one term", tt.size() == 1);
  g.result_slot = tt[0];
  const std::vector<double> got = run_grad(std::move(g), f2);
  for (size_t i = 0; i < want.size(); ++i)
    expect_close(("ark v" + std::to_string(i)).c_str(), got[i], want[i]);
}
```

- [ ] **Step 2: Run to verify it fails**

Run: `cmake --build build-rel -j --target test_reroll && ./build-rel/test_reroll`
Expected: FAIL — Task 1's code has no widening allowlist for OP_MUL/OP_ADD lane-local chains (regions with non-density, non-INDEX lane-varying ops bail), so `ark regions==1` fails.

- [ ] **Step 3: Implement widening**

In `reroll.cpp` add:

```cpp
bool is_widenable_eltwise(uint16_t oc) {
  switch (oc) {
    case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV:
      return true;  // eltwise_expr.cpp kernels shape-dispatch at runtime
    default:
      return false;
  }
}
```

and in the position-classification step, where Task 1 rejected non-density lane-varying ops, accept them when `is_widenable_eltwise(t.opcode)` and every input classified INVARIANT / CONST_LANES / LANE_LOCAL (INVARIANT inputs stay len-1 and broadcast). Their `op.out` is the len-L slot path that already exists in the rewrite.

Verify against `runtime/kernels/eltwise_expr.cpp` that each allowlisted opcode's forward AND backward handle `(len-1 op len-L)`, `(len-L op len-1)`, and `(len-L op len-L)` shapes — read the kernel, don't assume. If one doesn't, drop it from the allowlist and note it in the commit message.

- [ ] **Step 4: Run to verify it passes**

Run: `./build-rel/test_reroll` — expected `test_reroll OK`. Then `ctest --test-dir build-rel` — 17/17.

- [ ] **Step 5: Commit**

```bash
git add runtime/src/reroll.cpp tests/test_reroll.cpp
git commit -m "feat: re-roll arK shape - hoist invariant ops, widen elementwise lanes"
```

### Task 3: Bail-out safety tests (the pass must refuse what it can't prove)

**Files:**
- Modify: `tests/test_reroll.cpp`

**Interfaces:**
- Consumes: `reroll(...)`. No new surface.

- [ ] **Step 1: Write the tests** (append; each asserts `st.regions == 0` and `g.ops.size()` unchanged)

```cpp
// (a) cross-lane dependence: lane l reads lane l-1's output (a recurrence
// on parameters). Must NOT vectorize.
static void test_bail_recurrence() {
  const int L = 6;
  Graph g;
  Fills fills;
  const int sigma = g.add_slot(1, true);
  int prev = g.add_slot(1, true);  // x0 param
  std::vector<int> terms;
  for (int l = 0; l < L; ++l) {
    const int nx = g.add_slot(1, false);
    g.add_op(OP_MUL, {prev, sigma}, nx);
    const int lp = g.add_slot(1, false);
    const int id = g.add_op(OP_NORMAL_LPDF, {nx, prev, sigma}, lp);
    g.ops[id].variant = 0x06;
    terms.push_back(lp);
    prev = nx;  // <- lane l+1 reads lane l's out
  }
  std::vector<int> tt = terms;
  const size_t before = g.ops.size();
  RerollStats st = reroll(g, fills, tt);
  expect("recurrence not rerolled", st.regions == 0);
  expect("recurrence ops unchanged", g.ops.size() == before);
}

// (b) density out consumed by another op (gauss_mix shape): must bail.
static void test_bail_nonterm_density() {
  const int L = 6;
  Graph g;
  Fills fills;
  const int mu = g.add_slot(1, true);
  const int sigma = g.add_slot(1, true);
  auto cslot = [&](double v) {
    const int s = g.add_slot(1, false);
    fills.emplace_back(s, std::vector<double>{v});
    return s;
  };
  std::vector<int> terms;
  for (int l = 0; l < L; ++l) {
    const int lp = g.add_slot(1, false);
    const int id = g.add_op(OP_NORMAL_LPDF, {cslot(0.1 * l), mu, sigma}, lp);
    g.ops[id].variant = 0x06;
    const int doubled = g.add_slot(1, false);
    g.add_op(OP_ADD, {lp, lp}, doubled);  // lp escapes into an op
    terms.push_back(doubled);
  }
  std::vector<int> tt = terms;
  const size_t before = g.ops.size();
  RerollStats st = reroll(g, fills, tt);
  expect("nonterm density not rerolled", st.regions == 0);
  expect("nonterm ops unchanged", g.ops.size() == before);
}

// (c) partial-range INDEX progression (0..L-1 over a longer base): bail.
static void test_bail_partial_range() {
  const int L = 6;
  Graph g;
  Fills fills;
  const int sigma = g.add_slot(1, true);
  const int alpha = g.add_slot(1, true);
  const int base = g.add_slot(L + 3, false);  // longer than lane count
  g.add_op(OP_REP_VEC, {alpha}, base);
  auto cslot = [&](double v) {
    const int s = g.add_slot(1, false);
    fills.emplace_back(s, std::vector<double>{v});
    return s;
  };
  std::vector<int> terms;
  for (int l = 0; l < L; ++l) {
    const int idx = g.add_slot(1, false);
    g.add_op(OP_INDEX, {base}, idx, {l});
    const int lp = g.add_slot(1, false);
    const int id = g.add_op(OP_NORMAL_LPDF, {cslot(0.2 * l), idx, sigma}, lp);
    g.ops[id].variant = 0x06;
    terms.push_back(lp);
  }
  std::vector<int> tt = terms;
  const size_t before = g.ops.size();
  RerollStats st = reroll(g, fills, tt);
  expect("partial range not rerolled", st.regions == 0);
  expect("partial ops unchanged", g.ops.size() == before);
}

// (d) STANLI_NO_REROLL disables the pass.
static void test_env_disable() {
  setenv("STANLI_NO_REROLL", "1", 1);
  const int L = 6;
  Graph g;
  Fills fills;
  const int mu = g.add_slot(1, true);
  const int sigma = g.add_slot(1, true);
  std::vector<int> terms;
  for (int l = 0; l < L; ++l) {
    const int c = g.add_slot(1, false);
    fills.emplace_back(c, std::vector<double>{0.1 * l});
    const int lp = g.add_slot(1, false);
    const int id = g.add_op(OP_NORMAL_LPDF, {c, mu, sigma}, lp);
    g.ops[id].variant = 0x06;
    terms.push_back(lp);
  }
  std::vector<int> tt = terms;
  RerollStats st = reroll(g, fills, tt);
  expect("env disables", st.regions == 0);
  unsetenv("STANLI_NO_REROLL");
}
```

Add `#include <cstdlib>` for setenv. Call all four from `main()`.

- [ ] **Step 2: Run**

Run: `cmake --build build-rel -j --target test_reroll && ./build-rel/test_reroll`
Expected: all pass if Tasks 1-2 classification is correct. Any failure here is a real pass bug — fix the classifier, not the test. Likely first offender: (a) requires the lane-local check to reject producers from a *different* lane (the `out_of(pit->second, l)` comparison does this — verify it).

- [ ] **Step 3: Commit**

```bash
git add tests/test_reroll.cpp
git commit -m "test: re-roll bail-outs - recurrence, escaping density, partial range, env kill switch"
```

### Task 4: Wire into lowering + end-to-end fixtures

**Files:**
- Modify: `runtime/src/lower.cpp:2170-2179` (the `run()` method)
- Create: `tests/fixtures/rloop.stan`, `tests/fixtures/rloop.tmir.sexp`, `tests/fixtures/arloop.stan`, `tests/fixtures/arloop.tmir.sexp`
- Modify: `tools/gen_fixtures.sh` (add the two models, matching its existing per-fixture pattern)
- Modify: `tests/test_reroll.cpp` (E2E test)

**Interfaces:**
- Consumes: `reroll(...)`.
- Produces: `compile_model()` output is re-rolled by default; `STANLI_NO_REROLL=1` reproduces the old graph exactly.

- [ ] **Step 1: Write the fixtures**

`tests/fixtures/rloop.stan`:

```stan
data {
  int<lower=0> N;
  vector[N] x;
  vector[N] y;
}
parameters {
  real alpha;
  real beta;
  real<lower=0> sigma;
}
model {
  vector[N] mu;
  sigma ~ normal(0, 1);
  alpha ~ normal(0, 10);
  beta ~ normal(0, 10);
  mu = alpha + beta * x;
  for (n in 1 : N) {
    target += normal_lpdf(y[n] | mu[n], sigma);
  }
}
```

`tests/fixtures/arloop.stan`:

```stan
data {
  int<lower=0> K;
  int<lower=0> T;
  array[T] real y;
}
parameters {
  real alpha;
  array[K] real beta;
  real<lower=0> sigma;
}
model {
  alpha ~ normal(0, 10);
  beta ~ normal(0, 10);
  sigma ~ cauchy(0, 2.5);
  for (t in (K + 1) : T) {
    real mu;
    mu = alpha;
    for (k in 1 : K) {
      mu = mu + beta[k] * y[t - k];
    }
    y[t] ~ normal(mu, sigma);
  }
}
```

Generate the `.tmir.sexp` files the same way `tools/gen_fixtures.sh` does for existing fixtures (read the script first; it is the authority on the stanc invocation — expected shape: `deps/stanc3/stanc --debug-transformed-mir tests/fixtures/rloop.stan > tests/fixtures/rloop.tmir.sexp`), and add both models to the script so regeneration stays reproducible.

- [ ] **Step 2: Write the failing E2E test** (append to `tests/test_reroll.cpp`)

```cpp
#include <stanli/compile.hpp>
#include <fstream>
#include <sstream>

static std::string slurp(const char* p) {
  std::ifstream f(p);
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

static std::vector<double> e2e_grad(const char* sexp, const char* json) {
  DataMap data = DataMap::from_json(json);
  CompiledModel cm = compile_model(slurp(sexp), data);
  Executor ex(std::move(cm.graph));
  cm.bind(ex);
  for (int64_t i = 0; i < ex.n_params(); ++i)
    ex.params_data()[i] = 0.1 + 0.05 * (i % 7) - 0.15 * (i % 3);
  std::vector<double> out(1 + ex.n_params() + 1);
  out[0] = ex.gradient(out.data() + 1);
  out.back() = (double)0;  // placeholder, replaced below by op count probe
  return out;
}

static void test_e2e_fixtures() {
  // Small synthetic datasets; N/T large enough to trigger (>= kMinLanes).
  const char* rdata =
      "{\"N\":12,\"x\":[0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,1.0,1.1,1.2],"
      "\"y\":[1.1,0.9,1.3,0.7,1.0,1.2,0.8,1.05,0.95,1.15,0.85,1.0]}";
  const char* adata =
      "{\"K\":2,\"T\":12,\"y\":[0.3,0.5,0.2,0.6,0.4,0.55,0.35,0.45,0.5,"
      "0.42,0.48,0.44]}";
  struct Case { const char* sexp; const char* json; const char* name; };
  for (const Case& c : {Case{"tests/fixtures/rloop.tmir.sexp", rdata, "rloop"},
                        Case{"tests/fixtures/arloop.tmir.sexp", adata, "arloop"}}) {
    setenv("STANLI_NO_REROLL", "1", 1);
    const std::vector<double> want = e2e_grad(c.sexp, c.json);
    unsetenv("STANLI_NO_REROLL");
    const std::vector<double> got = e2e_grad(c.sexp, c.json);
    expect((std::string(c.name) + " sizes").c_str(),
           got.size() == want.size());
    for (size_t i = 0; i + 1 < want.size(); ++i)
      expect_close((std::string(c.name) + " v" + std::to_string(i)).c_str(),
                   got[i], want[i]);
  }
}
```

Also assert shrinkage: give `e2e_grad` an out-param `size_t* n_ops` filled from `cm.graph.ops.size()` before the move, and in the test require re-rolled `n_ops` < unrolled `n_ops / 4` for both fixtures. (Write it as a proper parameter, not the placeholder shown above.)

- [ ] **Step 3: Run to verify the E2E test fails**

Run: `./build-rel/test_reroll`
Expected: FAIL — `compile_model` does not call `reroll` yet, so op counts match and the shrinkage assertion fails.

- [ ] **Step 4: Wire the call site**

In `lower.cpp` `run()` (line ~2170), and add `#include <stanli/reroll.hpp>` at the top:

```cpp
  CompiledModel run(const mir::Program& p) {
    for (const auto& f : p.fun_defs) fun_defs[f.name] = &f;
    bind_data(p);
    for (const auto& s : p.log_prob) lower_stmt(s);
    reroll(g, out.fills, target_terms);   // no-op under STANLI_NO_REROLL=1
    info.resize(g.slots.size());          // keep SlotInfo parallel: emit()
                                          // in reduce_terms reads info[o]
    std::vector<int> all = target_terms;
    all.insert(all.end(), jac_slots.begin(), jac_slots.end());
    g.result_slot = reduce_terms(all);
    out.graph = std::move(g);
    return std::move(out);
  }
```

- [ ] **Step 5: Run tests**

Run: `cmake --build build-rel -j && ctest --test-dir build-rel`
Expected: 17/17 including the E2E shrinkage and parity assertions.

- [ ] **Step 6: Commit**

```bash
git add runtime/src/lower.cpp tests/fixtures/ tools/gen_fixtures.sh tests/test_reroll.cpp
git commit -m "feat: run re-roll pass in lowering; end-to-end loop fixtures"
```

### Task 5: Corpus verification, benchmarks, docs

**Files:**
- Modify: `docs/benchmarks.md`
- Test: full corpus + bench runs (no new test files)

**Interfaces:**
- Consumes: everything above. Produces numbers and docs only.

- [ ] **Step 1: Full corpus differential verification**

Run: `python3 tools/corpus.py deps/posteriordb` (read the script header first for the exact invocation the other agent uses; `tools/verify_sample.py` for per-model checks). Compare pass counts against the current baseline (119/120 evaluate, ~118 verified per docs/corpus-status.md).
Expected: no model regresses from verified to failing. Bitwise count MAY drop (re-rolled models change summation order) — record which models moved tiers. Any *verification* failure: bisect with `STANLI_NO_REROLL=1`; if the pass caused it, that model's region is a classifier bug — reduce it to a unit test in `tests/test_reroll.cpp` before fixing.

- [ ] **Step 2: Benchmark the three target models + regression sentinels**

Protocol (no core pinning exists on macOS; this is the substitute):
1. `ps aux -r | head -5` — note CPU-heavy neighbors (the other agent's builds).
2. `python3 spikes/bench_spike.py` still measures orig-vs-handvec; for the pass, run `tools/bench_models.py deps/cmdstan deps/posteriordb radon_pooled arK low_dim_gauss_mix eight_schools_noncentered bym2_offset_only` twice: once normally, once with `STANLI_NO_REROLL=1` — interleaved if the machine is noisy.
3. Accept if: radon_pooled ≥4x vs its no-reroll self, arK ≥10x, gauss_mix unchanged-or-better (its density lanes feed LOG_MIX and must bail in this phase), sentinels (eight_schools, bym2) within noise of no-reroll (their graphs contain no qualifying regions; the pass must not touch them — verify identical op counts via `dump_ops` if in doubt).

- [ ] **Step 3: Update docs/benchmarks.md**

Replace the stale "~400 ns for a scalar `normal_lpdf` op" claim with the measured decomposition (17-20 ns: ~9.5 executor + ~9 recorder + ~0.9 math; `tools/bench_opcost.cpp`). Refresh the per-gradient table with post-pass numbers and a one-paragraph description of the pass and its bail-outs. Note the bitwise-tier change for re-rolled models.

- [ ] **Step 4: Commit**

```bash
git add docs/benchmarks.md
git commit -m "docs: re-roll pass benchmarks; correct per-op cost figure"
```

### Out of scope (phase 2+, tracked in README roadmap)

- Batched `OP_LSE2`/`OP_LOG_MIX` + elementwise-lp density variant (unlocks low_dim_gauss_mix's remaining 2x+; the bail in Task 3(b) is its marker).
- Partial-range/strided INDEX progressions → `OP_SLICE`/`OP_SLICE_STRIDED`/`OP_GATHER` rewrites.
- Non-density term positions via `OP_SUM_VEC` (loops of `target += <elementwise expr>`).
- Bind-time `KernelCtx` precomputation (~20-30% on scalar-heavy graphs; measured in `tools/bench_opcost.cpp`).
- Merging to main: the OTHER AGENT owns main right now. Land this branch via rebase onto main's tip at merge time; expect conflicts only in `CMakeLists.txt` source lists and `lower.cpp:run()`.

## Self-review notes

- Spec coverage: detection, three input classes, INDEX elision, hoisting, widening, density-to-target, bail-outs, env escape, call-site info resize, fixtures, corpus, bench, docs — all have tasks.
- The Task 1 code sketch is a working skeleton with two marked must-fix lines; Task 1 Step 3 calls them out explicitly so they cannot ship.
- Type consistency: `reroll(Graph&, Fills&, std::vector<int>&)` used identically in Tasks 1-4; `Fills` alias defined in the test file only (production signature spells the pair type out).
