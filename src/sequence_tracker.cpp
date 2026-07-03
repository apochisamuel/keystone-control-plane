#include "internal.hpp"

namespace aegis::detail {

void SequenceTracker::observe(std::uint32_t session_id, std::uint32_t sequence) {
    auto& state=states_[session_id]; if(!state.initialized){state.last=sequence;state.initialized=true;return;} if(sequence>state.last+1U) state.gaps+=sequence-state.last-1U; if(sequence>state.last) state.last=sequence;
}

std::uint64_t SequenceTracker::gap_count(std::uint32_t session_id) const { const auto found=states_.find(session_id); return found==states_.end()?0U:found->second.gaps; }

std::uint32_t SequenceTracker::next_expected(std::uint32_t session_id) const { const auto found=states_.find(session_id); return found==states_.end()||!found->second.initialized?0U:found->second.last+1U; }

void SequenceTracker::clear() { states_.clear(); }

}  // namespace aegis::detail
