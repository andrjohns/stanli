// Loop re-rolling: lowering unrolls data-bound loops, so a scalar-loop
// model arrives here as N consecutive copies of a small op template whose
// only variation is (a) fresh output slots, (b) per-lane constant inputs
// from the dedup'd const pool, and (c) OP_INDEX immediates advancing
// 0,1,2,... The pass detects such regions and rewrites them into the
// vectorized ops the kernels already support, turning per-op dispatch and
// recorder overhead (~17-20ns per scalar op) into per-region cost.
//
// Template inputs classify as:
//   INVARIANT   same slot every lane -> keep; kernels broadcast len-1
//   CONST_LANES every lane a len-1 fill-backed const -> materialize a
//               constant vector from the VALUES (the pool is dedup'd, so
//               equal data values share slots; never assume slot runs)
//   LANE_LOCAL  the output of an earlier template position in the same
//               lane -> the corresponding vectorized output
// OP_INDEX positions with an invariant base either hoist (idata invariant
// across lanes) or vanish entirely (idata == lane number and the base has
// exactly lane-count elements: the vectorized consumer reads the base).
// A density whose every lane output is a target term becomes one vector
// density: the vector kernels already return the summed lp, which also
// deletes the region's share of the ADD_N reduction tree.
//
// Anything unclassifiable bails per-region, never per-model: cross-lane
// reads (parameter recurrences), partial/strided INDEX progressions,
// outputs escaping the lane, non-allowlisted opcodes.
#include <stanrt/optable.hpp>
#include <stanrt/reroll.hpp>

#include <cstdlib>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace stanrt {
namespace {

constexpr int kMinLanes = 4;
constexpr int kMaxPeriod = 32;

// Real-argument lpdfs whose vector instantiation returns the summed lp
// with per-element partials (densities.cpp bind_args shape dispatch).
bool is_density(uint16_t oc) {
  switch (oc) {
    case OP_NORMAL_LPDF:
    case OP_CAUCHY_LPDF:
    case OP_STUDENT_T_LPDF:
    case OP_GAMMA_LPDF:
    case OP_BETA_LPDF:
    case OP_LOGNORMAL_LPDF:
    case OP_UNIFORM_LPDF:
    case OP_DOUBLE_EXP_LPDF:
    case OP_EXPONENTIAL_LPDF:
    case OP_INV_GAMMA_LPDF:
    case OP_STD_NORMAL_LPDF:
      return true;
    default:
      return false;
  }
}

// Ops whose forward and backward shape-dispatch at runtime (len-1
// broadcasts), so widening scalar lanes to one vector op is the same
// opcode (eltwise_expr.cpp).
bool is_widenable(uint16_t oc) {
  switch (oc) {
    case OP_ADD:
    case OP_SUB:
    case OP_MUL:
    case OP_DIV:
      return true;
    default:
      return false;
  }
}

enum class InKind { kInvariant, kConstLanes, kLaneLocal, kBad };

struct PosIn {
  InKind kind = InKind::kBad;
  int producer_pos = -1;         // LANE_LOCAL: template position of producer
  std::vector<double> values;    // CONST_LANES: one value per lane
};

struct Pos {
  std::vector<PosIn> ins;
  bool index_elision = false;  // OP_INDEX, idata==lane, base len==lanes
  bool hoist = false;          // all inputs + idata invariant: emit once
  bool term_density = false;   // density, every lane's out a target term
};

// Structural template match; idata may differ across lanes only for
// OP_INDEX (checked as a progression during classification).
bool ops_match(const Graph& g, const Op& a, const Op& b) {
  if (a.opcode != b.opcode || a.variant != b.variant || a.n_in != b.n_in ||
      a.out2 >= 0 || b.out2 >= 0)
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

}  // namespace

RerollStats reroll(Graph& g,
                   std::vector<std::pair<int, std::vector<double>>>& fills,
                   std::vector<int>& target_terms) {
  RerollStats st;
  st.ops_before = static_cast<int64_t>(g.ops.size());
  st.ops_after = st.ops_before;
  if (std::getenv("STANRT_NO_REROLL")) return st;

  // The dedup'd constant pool: slot -> value, for len-1 fills.
  std::unordered_map<int, double> const_val;
  for (const auto& f : fills)
    if (f.second.size() == 1) const_val.emplace(f.first, f.second[0]);

  // Consumers of each slot, by original op index. Ops only read slots
  // produced earlier, so indices stay valid as the scan rewrites disjoint
  // regions left to right.
  std::unordered_map<int, std::vector<size_t>> uses;
  for (size_t u = 0; u < g.ops.size(); ++u)
    for (int j = 0; j < g.ops[u].n_in; ++j)
      uses[g.ops[u].in[j]].push_back(u);

  std::unordered_set<int> term_set(target_terms.begin(), target_terms.end());

  std::vector<Op> result;
  result.reserve(g.ops.size());
  size_t i = 0;
  while (i < g.ops.size()) {
    bool rewrote = false;
    for (int P = 1; P <= kMaxPeriod && i + 2 * (size_t)P <= g.ops.size();
         ++P) {
      // Count template-matching lanes.
      int L = 1;
      while (i + ((size_t)L + 1) * P <= g.ops.size()) {
        bool match = true;
        for (int p = 0; p < P && match; ++p)
          match = ops_match(g, g.ops[i + p], g.ops[i + (size_t)L * P + p]);
        if (!match) break;
        ++L;
      }
      if (L < kMinLanes) continue;

      const auto op_at = [&](int p, int l) -> const Op& {
        return g.ops[i + (size_t)l * P + p];
      };

      // ---- classify ----
      std::vector<Pos> pos((size_t)P);
      std::unordered_map<int, int> lane0_producer;  // lane-0 out -> position
      bool ok = true;
      bool any_term_density = false;
      for (int p = 0; p < P && ok; ++p) {
        const Op& t = op_at(p, 0);
        Pos& ap = pos[p];
        ap.ins.resize(t.n_in);
        bool all_inputs_invariant = true;
        for (int j = 0; j < t.n_in && ok; ++j) {
          bool invariant = true;
          for (int l = 1; l < L && invariant; ++l)
            invariant = op_at(p, l).in[j] == t.in[j];
          if (invariant) {
            ap.ins[j].kind = InKind::kInvariant;
            continue;
          }
          all_inputs_invariant = false;
          auto pit = lane0_producer.find(t.in[j]);
          if (pit != lane0_producer.end()) {
            bool local = true;
            for (int l = 1; l < L && local; ++l)
              local = op_at(p, l).in[j] == op_at(pit->second, l).out;
            if (local) {
              ap.ins[j].kind = InKind::kLaneLocal;
              ap.ins[j].producer_pos = pit->second;
              continue;
            }
          }
          std::vector<double> vals((size_t)L);
          bool all_const = true;
          for (int l = 0; l < L && all_const; ++l) {
            auto cit = const_val.find(op_at(p, l).in[j]);
            if (cit == const_val.end())
              all_const = false;
            else
              vals[(size_t)l] = cit->second;
          }
          if (all_const) {
            ap.ins[j].kind = InKind::kConstLanes;
            ap.ins[j].values = std::move(vals);
            continue;
          }
          ok = false;  // cross-lane read or otherwise unclassifiable
        }
        if (!ok) break;

        // Output discipline. A lane's output may be consumed only by later
        // ops of the same lane instance; density outputs may instead be
        // target terms (and then must have no op consumers at all).
        bool outs_are_terms = true;
        bool outs_lane_internal = true;
        for (int l = 0; l < L; ++l) {
          const int o = op_at(p, l).out;
          if (!term_set.count(o)) outs_are_terms = false;
          auto uit = uses.find(o);
          if (uit == uses.end()) continue;
          for (size_t u : uit->second) {
            const bool inside = u > i + (size_t)l * P + p &&
                                u < i + ((size_t)l + 1) * P;
            if (!inside) outs_lane_internal = false;
          }
        }

        // Position-level classification.
        if (t.opcode == OP_INDEX) {
          if (ap.ins[0].kind != InKind::kInvariant || !outs_lane_internal ||
              term_set.count(t.out)) {
            ok = false;
            break;
          }
          bool idata_invariant = true, progression = true;
          for (int l = 0; l < L; ++l) {
            const int v = op_at(p, l).idata[0];
            if (v != t.idata[0]) idata_invariant = false;
            if (v != l) progression = false;
          }
          if (progression && g.slots[t.in[0]].len == L) {
            ap.index_elision = true;
          } else if (idata_invariant) {
            ap.hoist = true;
          } else {
            ok = false;  // partial or strided progression: bail (v1)
          }
        } else if (is_density(t.opcode) && outs_are_terms) {
          auto uit = uses.find(t.out);
          if (uit != uses.end() && !uit->second.empty()) {
            ok = false;  // a term that is also an op input: leave alone
          } else {
            ap.term_density = true;
            any_term_density = true;
          }
        } else if (all_inputs_invariant && !term_set.count(t.out) &&
                   outs_lane_internal) {
          ap.hoist = true;
        } else if (is_widenable(t.opcode) && !term_set.count(t.out) &&
                   outs_lane_internal) {
          // widened in the rewrite below
        } else {
          ok = false;
        }
        if (ok) lane0_producer[t.out] = p;
      }
      if (!ok || !any_term_density) continue;

      // ---- rewrite ----
      std::vector<int> pos_out((size_t)P, -1);
      for (int p = 0; p < P; ++p) {
        const Op& t = op_at(p, 0);
        Pos& ap = pos[p];
        if (ap.index_elision) {
          pos_out[(size_t)p] = t.in[0];
          continue;
        }
        if (ap.hoist) {
          result.push_back(t);
          pos_out[(size_t)p] = t.out;
          continue;
        }
        Op op = t;  // opcode, variant, idata carry over
        for (int j = 0; j < t.n_in; ++j) {
          switch (ap.ins[j].kind) {
            case InKind::kInvariant:
              break;
            case InKind::kLaneLocal:
              op.in[j] = pos_out[(size_t)ap.ins[j].producer_pos];
              break;
            case InKind::kConstLanes: {
              const int cs = g.add_slot(L, false);
              fills.emplace_back(cs, ap.ins[j].values);
              op.in[j] = cs;
              break;
            }
            case InKind::kBad:
              break;  // unreachable: classification succeeded
          }
        }
        if (ap.term_density) {
          op.out = g.add_slot(1, false);
          // Swap the L lane terms for the one summed term, at the first
          // lane's position.
          std::unordered_set<int> dead;
          for (int l = 0; l < L; ++l) dead.insert(op_at(p, l).out);
          std::vector<int> next_terms;
          next_terms.reserve(target_terms.size());
          bool placed = false;
          for (int s : target_terms) {
            if (dead.count(s)) {
              if (!placed) {
                next_terms.push_back(op.out);
                placed = true;
              }
            } else {
              next_terms.push_back(s);
            }
          }
          target_terms = std::move(next_terms);
          for (int s : dead) term_set.erase(s);
          term_set.insert(op.out);
        } else {
          op.out = g.add_slot(L, false);
        }
        pos_out[(size_t)p] = op.out;
        result.push_back(op);
      }
      i += (size_t)P * L;
      ++st.regions;
      rewrote = true;
      break;
    }
    if (!rewrote) {
      result.push_back(g.ops[i]);
      ++i;
    }
  }
  g.ops = std::move(result);
  st.ops_after = static_cast<int64_t>(g.ops.size());
  return st;
}

}  // namespace stanrt
