#include "internal.hpp"

namespace aegis::detail {

std::uint64_t RoutingSnapshotStore::capture(std::uint32_t session_id, std::uint64_t generation, std::vector<Route> routes) { const auto id=next_id_++; history_[session_id].push_back({id,session_id,generation,std::move(routes)}); return id; }

std::optional<RoutingSnapshot> RoutingSnapshotStore::latest(std::uint32_t session_id) const { const auto found=history_.find(session_id); if(found==history_.end()||found->second.empty()) return std::nullopt; return found->second.back(); }

std::size_t RoutingSnapshotStore::prune(std::uint32_t session_id, std::size_t keep) { auto found=history_.find(session_id); if(found==history_.end() || found->second.size()<=keep) return 0U; const auto count=found->second.size()-keep; found->second.erase(found->second.begin(),found->second.begin()+static_cast<std::ptrdiff_t>(count)); return count; }

void RoutingSnapshotStore::clear() { history_.clear(); next_id_=1U; }

}  // namespace aegis::detail
