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
// Failed classifications report the longest still-classifiable lane
// prefix and retry with it. This is what handles block-structured data
// (rats_model: obs sorted time-major, so INDEX idata restarts 0..29 every
// time block): each block classifies as its own region and the scan
// resumes at the block boundary. Anything unclassifiable bails per-region,
// never per-model: cross-lane reads (parameter recurrences), partial or
// strided INDEX progressions, outputs escaping the lane, opcodes outside
// the vocabulary.
#include <stanli/optable.hpp>
#include <stanli/reroll.hpp>

#include <algorithm>
#include <cstdlib>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace stanli {
namespace {

constexpr int64_t kMinLanes = 4;
constexpr int kMaxPeriod = 32;
constexpr int kMaxClassifyAttempts = 6;

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
  int slice_start = -1;        // OP_INDEX over a contiguous window
  std::vector<int> gather_idx; // OP_INDEX with a data-driven index
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
                   std::vector<int>& target_terms,
                   const std::vector<int>& extra_roots) {
  RerollStats st;
  st.ops_before = static_cast<int64_t>(g.ops.size());
  st.ops_after = st.ops_before;
  if (std::getenv("STANLI_NO_REROLL")) return st;

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

  // Slots read from outside the op graph (jacobian terms, constrained
  // parameter views). They have no consuming op, so `uses` cannot see
  // them; folding a lane that writes one would leave it unwritten.
  std::unordered_set<int> root_set(extra_roots.begin(), extra_roots.end());

  // Scan-cost control: after a hard classification failure (prefix 0,
  // lane-independent evidence) the run gets one more attempt one lane in,
  // then is skipped wholesale for that period. Soft failures (positive
  // prefix) always re-attempt at the reported boundary. Without this,
  // graphs made of enormous near-periodic runs go quadratic (ldaK5:
  // 1.03M ops in 33k-lane log_sum_exp lanes hung the pass; its runs are
  // now pruned by the density pre-check before lane counting).
  std::vector<size_t> retry_at((size_t)kMaxPeriod + 1, 0);
  std::vector<size_t> fail_end((size_t)kMaxPeriod + 1, 0);
  std::vector<bool> hard_failed((size_t)kMaxPeriod + 1, false);

  std::vector<Op> result;
  result.reserve(g.ops.size());
  size_t i = 0;
  while (i < g.ops.size()) {
    bool rewrote = false;
    for (int P = 1; P <= kMaxPeriod && i + 2 * (size_t)P <= g.ops.size();
         ++P) {
      if (i < retry_at[(size_t)P]) continue;
      // Cheap pre-check before any lane counting: a profitable region
      // must contain an allowlisted density whose out is a target term.
      bool candidate = false;
      for (int p = 0; p < P && !candidate; ++p) {
        const Op& t = g.ops[i + p];
        candidate = is_density(t.opcode) && term_set.count(t.out) != 0;
      }
      if (!candidate) continue;

      // Count template-matching lanes.
      int64_t L = 1;
      while (i + ((size_t)L + 1) * P <= g.ops.size()) {
        bool match = true;
        for (int p = 0; p < P && match; ++p)
          match = ops_match(g, g.ops[i + p], g.ops[i + (size_t)L * P + p]);
        if (!match) break;
        ++L;
      }
      if (L < kMinLanes) continue;

      const auto op_at = [&](int p, int64_t l) -> const Op& {
        return g.ops[i + (size_t)l * P + p];
      };

      // ---- classify, shrinking to the reported prefix on failure ----
      std::vector<Pos> pos;
      int64_t Luse = L;
      bool classified = false;
      for (int attempt = 0;
           attempt < kMaxClassifyAttempts && Luse >= kMinLanes; ++attempt) {
        int64_t prefix = Luse;
        pos.assign((size_t)P, Pos{});
        std::unordered_map<int, int> lane0_producer;
        bool ok = true;
        bool any_term_density = false;
        for (int p = 0; p < P; ++p) {
          const Op& t = op_at(p, 0);
          Pos& ap = pos[p];
          ap.ins.resize(t.n_in);
          bool all_inputs_invariant = true;
          for (int j = 0; j < t.n_in; ++j) {
            // Longest lane prefix under each interpretation; pick the
            // interpretation valid for all Luse lanes, else bound prefix.
            int64_t br_inv = Luse;
            for (int64_t l = 1; l < Luse; ++l)
              if (op_at(p, l).in[j] != t.in[j]) {
                br_inv = l;
                break;
              }
            if (br_inv == Luse) {
              ap.ins[j].kind = InKind::kInvariant;
              continue;
            }
            all_inputs_invariant = false;
            int64_t br_local = 0;
            auto pit = lane0_producer.find(t.in[j]);
            if (pit != lane0_producer.end()) {
              br_local = Luse;
              for (int64_t l = 1; l < Luse; ++l)
                if (op_at(p, l).in[j] != op_at(pit->second, l).out) {
                  br_local = l;
                  break;
                }
              if (br_local == Luse) {
                ap.ins[j].kind = InKind::kLaneLocal;
                ap.ins[j].producer_pos = pit->second;
                continue;
              }
            }
            int64_t br_const = Luse;
            std::vector<double> vals;
            vals.reserve((size_t)Luse);
            for (int64_t l = 0; l < Luse; ++l) {
              auto cit = const_val.find(op_at(p, l).in[j]);
              if (cit == const_val.end()) {
                br_const = l;
                break;
              }
              vals.push_back(cit->second);
            }
            if (br_const == Luse) {
              ap.ins[j].kind = InKind::kConstLanes;
              ap.ins[j].values = std::move(vals);
              continue;
            }
            ok = false;
            prefix =
                std::min(prefix, std::max({br_inv, br_local, br_const}));
          }

          // Output discipline prefixes. A lane's out may be consumed only
          // by later ops of its own lane instance; density outs may
          // instead be target terms (with no op consumers at all).
          int64_t br_term = Luse;     // lanes whose out IS a term
          int64_t br_nonterm = Luse;  // lanes whose out is NOT a term
          int64_t br_internal = Luse; // lanes whose out does not escape
          for (int64_t l = 0; l < Luse; ++l) {
            const int o = op_at(p, l).out;
            const bool is_term = term_set.count(o) != 0;
            if (!is_term && br_term == Luse) br_term = l;
            if (is_term && br_nonterm == Luse) br_nonterm = l;
            if (br_internal == Luse && root_set.count(o) != 0) br_internal = l;
            if (br_internal == Luse) {
              auto uit = uses.find(o);
              if (uit != uses.end())
                for (size_t u : uit->second) {
                  const bool inside = u > i + (size_t)l * P + p &&
                                      u < i + ((size_t)l + 1) * P;
                  if (!inside) {
                    br_internal = l;
                    break;
                  }
                }
            }
          }

          // Position-level classification.
          if (t.opcode == OP_INDEX) {
            int64_t br_prog = Luse, br_iinv = Luse;
            for (int64_t l = 0; l < Luse; ++l) {
              const int v = op_at(p, l).idata[0];
              if (v != l && br_prog == Luse) br_prog = l;
              if (v != t.idata[0] && br_iinv == Luse) br_iinv = l;
            }
            const int64_t blen = g.slots[t.in[0]].len;
            const int64_t io_ok = std::min(br_internal, br_nonterm);
            // Contiguous ascending run (idata[l] == idata[0] + l) and
            // in-range indices: the two cheaper rewrites below.
            int64_t br_run = Luse;
            bool in_range = true;
            for (int64_t l = 0; l < Luse; ++l) {
              const int v = op_at(p, l).idata[0];
              if (v != t.idata[0] + l && br_run == Luse) br_run = l;
              if (v < 0 || v >= blen) in_range = false;
            }
            if (ap.ins[0].kind != InKind::kInvariant || io_ok < Luse) {
              ok = false;
              prefix = std::min(prefix, io_ok);
            } else if (br_prog == Luse && blen == Luse) {
              ap.index_elision = true;  // reads the whole base, in order
            } else if (br_iinv == Luse) {
              ap.hoist = true;  // same element every lane
            } else if (br_run == Luse && t.idata[0] + Luse <= blen) {
              ap.slice_start = t.idata[0];  // contiguous window -> OP_SLICE
            } else if (in_range) {
              // Arbitrary data-driven index (`alpha[county_idx[n]]`, the
              // hierarchical idiom) -> one OP_GATHER over the lane indices.
              ap.gather_idx.reserve((size_t)Luse);
              for (int64_t l = 0; l < Luse; ++l)
                ap.gather_idx.push_back(op_at(p, l).idata[0]);
            } else {
              ok = false;
              prefix = 0;  // out-of-range index: not ours to rewrite
            }
          } else if (is_density(t.opcode)) {
            if (br_term < Luse || br_internal < Luse) {
              // br_internal here can only mean an extra root: a term
              // density has no op consumers to escape to.
              ok = false;
              prefix = std::min(prefix, std::min(br_term, br_internal));
            } else {
              auto uit = uses.find(t.out);
              if (uit != uses.end() && !uit->second.empty()) {
                ok = false;
                prefix = 0;  // a term that is also an op input
              } else {
                ap.term_density = true;
                any_term_density = true;
              }
            }
          } else if (all_inputs_invariant) {
            const int64_t io_ok = std::min(br_internal, br_nonterm);
            if (io_ok == Luse) {
              ap.hoist = true;
            } else {
              ok = false;
              prefix = std::min(prefix, io_ok);
            }
          } else if (is_widenable(t.opcode)) {
            const int64_t io_ok = std::min(br_internal, br_nonterm);
            if (io_ok < Luse) {
              ok = false;
              prefix = std::min(prefix, io_ok);
            }
          } else {
            ok = false;
            prefix = 0;  // opcode outside the vocabulary: no prefix helps
          }
          lane0_producer[t.out] = p;
        }
        if (ok && any_term_density) {
          classified = true;
          break;
        }
        if (ok && !any_term_density) prefix = 0;  // classifiable but useless
        if (prefix >= Luse) prefix = Luse - 1;    // guarantee progress
        Luse = prefix;
      }

      if (!classified) {
        // Bookkeeping. Soft failures (positive prefix) re-attempt at the
        // reported boundary; hard failures (prefix 0) get one second
        // chance one lane in, then the whole run is skipped.
        const size_t run_end = i + (size_t)P * (size_t)L;
        if (Luse > 0) {
          retry_at[(size_t)P] = i + (size_t)P * (size_t)Luse;
          hard_failed[(size_t)P] = false;
        } else if (hard_failed[(size_t)P] && i < fail_end[(size_t)P]) {
          retry_at[(size_t)P] = fail_end[(size_t)P];
        } else {
          fail_end[(size_t)P] = run_end;
          hard_failed[(size_t)P] = true;
          retry_at[(size_t)P] = i + (size_t)P;
        }
        continue;
      }

      // ---- rewrite the classified prefix [i, i + P*Luse) ----
      std::vector<int> pos_out((size_t)P, -1);
      for (int p = 0; p < P; ++p) {
        const Op& t = op_at(p, 0);
        Pos& ap = pos[(size_t)p];
        if (ap.index_elision) {
          pos_out[(size_t)p] = t.in[0];
          continue;
        }
        if (ap.hoist) {
          result.push_back(t);
          pos_out[(size_t)p] = t.out;
          continue;
        }
        if (ap.slice_start >= 0 || !ap.gather_idx.empty()) {
          // One vector read replaces the lanes' scalar reads. Both kernels
          // scatter their adjoints back into the base, gather in ascending
          // lane order so repeated indices accumulate like the var path.
          Op rd;
          rd.opcode = ap.slice_start >= 0 ? OP_SLICE : OP_GATHER;
          rd.n_in = 1;
          rd.in[0] = t.in[0];
          rd.out = g.add_slot(Luse, false);
          std::vector<int> idata;
          if (ap.slice_start >= 0)
            idata.push_back(ap.slice_start);
          else
            idata = std::move(ap.gather_idx);
          g.idata_pool.push_back(std::move(idata));
          rd.idata = g.idata_pool.back().data();
          rd.n_idata = (int64_t)g.idata_pool.back().size();
          pos_out[(size_t)p] = rd.out;
          result.push_back(rd);
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
              const int cs = g.add_slot(Luse, false);
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
          // Swap the Luse lane terms for the one summed term, at the
          // first lane's position.
          std::unordered_set<int> dead;
          for (int64_t l = 0; l < Luse; ++l) dead.insert(op_at(p, l).out);
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
          op.out = g.add_slot(Luse, false);
        }
        pos_out[(size_t)p] = op.out;
        result.push_back(op);
      }
      i += (size_t)P * (size_t)Luse;
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

}  // namespace stanli
