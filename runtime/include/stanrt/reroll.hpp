// Re-roll pass: rewrites unrolled-loop regions (periodic op templates over
// consecutive lanes) into vectorized ops. Runs after lowering, before the
// target-term reduction, so N scalar density terms can become one summed
// vector-density term.
#ifndef STANRT_REROLL_HPP
#define STANRT_REROLL_HPP

#include <stanrt/graph.hpp>

#include <cstdint>
#include <utility>
#include <vector>

namespace stanrt {

struct RerollStats {
  int regions = 0;
  int64_t ops_before = 0;
  int64_t ops_after = 0;
};

// In place. `fills` gains entries for materialized constant vectors.
// `target_terms` entries produced by vectorized densities are replaced by
// the single summed output slot (at the first lane's position). Slots are
// appended to g.slots; callers with arrays parallel to slots must resize.
// STANRT_NO_REROLL=1 disables the pass.
RerollStats reroll(Graph& g,
                   std::vector<std::pair<int, std::vector<double>>>& fills,
                   std::vector<int>& target_terms);

}  // namespace stanrt

#endif
