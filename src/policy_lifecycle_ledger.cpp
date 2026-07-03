#include "internal.hpp"

namespace aegis::detail {

void PolicyLifecycleLedger::note(std::uint32_t session_id,std::uint64_t version,std::string phase,std::uint64_t tick){history_[session_id].push_back({session_id,version,std::move(phase),tick});}

std::vector<PolicyLifecycleEntry> PolicyLifecycleLedger::history(std::uint32_t session_id) const {const auto found=history_.find(session_id);return found==history_.end()?std::vector<PolicyLifecycleEntry>{}:found->second;}

std::optional<PolicyLifecycleEntry> PolicyLifecycleLedger::latest(std::uint32_t session_id) const {const auto found=history_.find(session_id);if(found==history_.end()||found->second.empty())return std::nullopt;return found->second.back();}

void PolicyLifecycleLedger::clear(){history_.clear();}

}  // namespace aegis::detail
