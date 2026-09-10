// A record of the island carver's choices, so a decision can be replayed
// with one candidate flipped and its compile reused.
#ifndef STANLI_CARVE_PLAN_HPP
#define STANLI_CARVE_PLAN_HPP

#include <stanli/kernel_types.hpp>

#include <cstddef>
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

struct CandidateRecord {
  CandidateKey key;
  CarveDecision taken = CarveDecision::kLeave;
  bool island_viable = false;
  bool split_viable = false;
};

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
