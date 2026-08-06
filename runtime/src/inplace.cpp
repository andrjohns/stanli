// Destructive functional updates.
//
// `mu[n] = ...` inside a data-bound loop unrolls into a chain of
// OP_SET_INDEX ops. Each one copies its whole input vector into a fresh
// slot and then pokes one element, so N writes cost O(N^2) time and O(N^2)
// arena. Measured on radon_county_intercept (N=12,573): 90.5 ms per
// gradient and 2.58 GB peak RSS, against CmdStan's 438 us.
//
// A write can mutate its input vector directly when nothing later observes
// that vector's pre-write contents. The condition is LAST USE, not single
// use: the read-back `INDEX(mu_n, n)` that immediately follows a write in
// the same lane is an earlier use of the same slot, so a single-use rule
// would refuse exactly the chains that matter. A write qualifies when
//
//   - its input is produced by an op. Fill-backed slots (declared vectors,
//     constants) are written once at bind time, so mutating one would let
//     state leak from one gradient evaluation into the next. The first
//     write of each chain therefore keeps its copy -- one O(N) copy per
//     evaluation, which is the price of the whole chain now.
//   - this op is the last op reading that slot, and
//   - neither the input nor the output is a root (read straight from the
//     arena, invisible in the use map) or a parameter.
//
// The rewrite makes the op's output slot BE its input slot, so `bind_()`
// gives them one offset with no executor change, and later references to
// the dead output slot are renamed to the input.
#include <stanli/inplace.hpp>
#include <stanli/optable.hpp>

#include <cstdlib>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace stanli {

int make_inplace_updates(Graph& g, const std::vector<int>& roots) {
  if (std::getenv("STANLI_NO_INPLACE")) return 0;

  const int n_slots = static_cast<int>(g.slots.size());
  // Last op index that reads each slot, and whether an op writes it.
  std::vector<int> last_use(n_slots, -1);
  std::vector<char> written(n_slots, 0);
  for (size_t i = 0; i < g.ops.size(); ++i) {
    for (int j = 0; j < g.ops[i].n_in; ++j)
      last_use[g.ops[i].in[j]] = static_cast<int>(i);
    written[g.ops[i].out] = 1;
    if (g.ops[i].out2 >= 0) written[g.ops[i].out2] = 1;
  }
  std::unordered_set<int> root_set(roots.begin(), roots.end());
  if (g.result_slot >= 0) root_set.insert(g.result_slot);

  std::unordered_map<int, int> rename;  // dead output slot -> live slot
  const auto resolve = [&](int s) {
    auto it = rename.find(s);
    return it == rename.end() ? s : it->second;
  };

  int n_rewritten = 0;
  for (size_t i = 0; i < g.ops.size(); ++i) {
    Op& op = g.ops[i];
    for (int j = 0; j < op.n_in; ++j) op.in[j] = resolve(op.in[j]);
    if (op.opcode != OP_SET_INDEX) continue;

    const int vec = op.in[0];
    // The renamed slot inherits the original's last use: a read of any
    // link in the chain is a read of the one buffer they now share.
    if (!written[vec] || g.slots[vec].is_param) continue;
    if (root_set.count(vec) || root_set.count(op.out)) continue;
    if (last_use[vec] != static_cast<int>(i)) continue;

    rename[op.out] = vec;
    // A later read of the old output is a read of the shared buffer, so
    // the surviving slot inherits it: the next write in the chain must not
    // treat itself as the last use while that read is still pending.
    last_use[vec] = std::max(last_use[vec], last_use[op.out]);
    // The old output slot is now unreachable: no op writes or reads it,
    // and it was neither a root nor fill-backed (checked above), so it
    // needs no arena. Without this the chain still costs O(N^2) memory --
    // bind_() sizes the arenas from slot lengths, not from op references.
    g.slots[op.out].len = 0;
    op.opcode = OP_SET_INDEX_INPLACE;
    op.out = vec;  // out and in[0] are now one slot: one buffer, one adjoint
    ++n_rewritten;
  }
  return n_rewritten;
}

}  // namespace stanli
