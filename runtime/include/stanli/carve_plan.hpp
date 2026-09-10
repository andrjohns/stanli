// A record of the island carver's choices, so a decision can be replayed
// with one candidate flipped and its compile reused.
#ifndef STANLI_CARVE_PLAN_HPP
#define STANLI_CARVE_PLAN_HPP

#include <stanli/kernel_types.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace stanli {

// A span [begin, end) of the pre-island op list, at the vocabulary (strict
// or not) the carver priced it with.
struct CandidateKey {
  size_t begin = 0;
  size_t end = 0;
  bool strict = false;
};

inline bool operator<(const CandidateKey& a, const CandidateKey& b) {
  if (a.begin != b.begin) return a.begin < b.begin;
  if (a.end != b.end) return a.end < b.end;
  return a.strict < b.strict;
}

enum class CarveDecision { kIsland, kSplit, kLeave };

// kUnknown when the natural path never priced that side of the decision.
enum class Viability { kUnknown, kNo, kYes };

struct CandidateRecord {
  CandidateKey key;
  CarveDecision taken = CarveDecision::kLeave;
  Viability island_viable = Viability::kUnknown;
  Viability split_viable = Viability::kUnknown;
  // The two costs the estimate compared to reach `taken`: island-vs-leave is
  // island_cost + boundary versus graph_cost, join-vs-split is the joined
  // cost versus the split cost. 0 when the natural path never priced a side.
  int64_t chosen_cost = 0;
  int64_t other_cost = 0;
};

// kUnknown is never eligible for tuning: the carver's own guard bounds
// already decided that side wasn't worth pricing, so it isn't worth timing.
inline bool desired_decision(const CandidateRecord& rec, CarveDecision* out) {
  if (rec.taken == CarveDecision::kIsland) {
    *out = CarveDecision::kLeave;
    return true;
  }
  if (rec.island_viable == Viability::kYes) {
    *out = CarveDecision::kIsland;
    return true;
  }
  return false;
}

inline double decision_closeness(const CandidateRecord& rec) {
  const int64_t mx = std::max(rec.chosen_cost, rec.other_cost);
  if (mx <= 0) return 0.0;
  return std::abs(static_cast<double>(rec.chosen_cost - rec.other_cost)) /
         static_cast<double>(mx);
}

inline bool has_eligible_choice(const std::vector<CandidateRecord>& decisions,
                                double trust_radius) {
  CarveDecision unused;
  for (const CandidateRecord& rec : decisions)
    if (desired_decision(rec, &unused) &&
        decision_closeness(rec) <= trust_radius)
      return true;
  return false;
}

// Indices of the eligible decisions in `decisions`, closest call first.
inline std::vector<size_t> eligible_decisions_by_closeness(
    const std::vector<CandidateRecord>& decisions) {
  std::vector<size_t> idx;
  for (size_t i = 0; i < decisions.size(); ++i) {
    CarveDecision unused;
    if (desired_decision(decisions[i], &unused)) idx.push_back(i);
  }
  std::stable_sort(idx.begin(), idx.end(), [&](size_t a, size_t b) {
    return decision_closeness(decisions[a]) < decision_closeness(decisions[b]);
  });
  return idx;
}

// Compiled candidates keyed by CandidateKey, shared across replays of the
// same plan. Candidate itself is file-local to island.cpp, so entries hold
// it type-erased alongside the op fields it was compiled from.
struct CarveCache {
  int compiles = 0;
  struct Entry {
    std::shared_ptr<void> candidate;
    std::vector<int> op_signature;
  };
  std::map<CandidateKey, Entry> entries;
};

struct CarvePlan {
  std::map<CandidateKey, CarveDecision> overrides;
  std::vector<CandidateRecord> decisions;
  // The op list carve_islands replaced, retained so a caller can rebuild the
  // pre-island graph and carve it again with a different override.
  std::vector<Op> pre_island_ops;
  std::shared_ptr<CarveCache> cache;
};

}  // namespace stanli

#endif
