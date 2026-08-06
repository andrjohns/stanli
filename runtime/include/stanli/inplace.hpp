// Destructive functional updates. `v[n] = ...` under an unrolled loop
// lowers to a chain of OP_SET_INDEX ops, each of which copies the whole
// vector into a fresh slot: N writes cost O(N^2) time and arena. When a
// write is the last use of the vector it overwrites, it can mutate that
// vector in place instead, making the chain O(N).
#ifndef STANLI_INPLACE_HPP
#define STANLI_INPLACE_HPP

#include <stanli/graph.hpp>

#include <vector>

namespace stanli {

// Rewrites eligible OP_SET_INDEX ops to OP_SET_INDEX_INPLACE (whose out
// slot IS its first input) and renames later references to the dead
// output slot. `roots` are slots read from outside the op graph (jacobian
// terms, constrained-parameter views, the result): they are never
// overwritten and never renamed. Returns the number of writes rewritten.
// STANLI_NO_INPLACE=1 disables the pass.
int make_inplace_updates(Graph& g, const std::vector<int>& roots);

}  // namespace stanli

#endif
