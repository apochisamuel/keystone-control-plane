#include "internal.hpp"

namespace aegis::detail {

void TopologyCache::put(std::uint32_t session_id,TopologyGraph graph){graphs_[session_id]=std::move(graph);}

std::optional<TopologyGraph> TopologyCache::get(std::uint32_t session_id) const {const auto found=graphs_.find(session_id);return found==graphs_.end()?std::nullopt:std::optional<TopologyGraph>(found->second);}

bool TopologyCache::invalidate(std::uint32_t session_id){return graphs_.erase(session_id)!=0U;}

void TopologyCache::clear(){graphs_.clear();}

}  // namespace aegis::detail
