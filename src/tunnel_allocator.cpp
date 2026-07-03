#include "internal.hpp"

namespace aegis::detail {

std::optional<TunnelLease> TunnelAllocator::allocate(std::uint32_t peer, std::uint64_t generation,
                                                      std::string endpoint) {
    endpoint = canonical_text(endpoint);
    if (peer == 0U || endpoint.empty() || next_id_ == std::numeric_limits<std::uint32_t>::max()) return std::nullopt;
    TunnelLease lease;
    lease.tunnel_id = next_id_++;
    lease.peer = peer;
    lease.generation = generation;
    lease.endpoint = std::move(endpoint);
    leases_.emplace(lease.tunnel_id, lease);
    return lease;
}

bool TunnelAllocator::release(std::uint32_t tunnel_id) { return leases_.erase(tunnel_id) != 0U; }

std::vector<TunnelLease> TunnelAllocator::active() const {
    std::vector<TunnelLease> result;
    for (const auto& [id, lease] : leases_) { (void)id; result.push_back(lease); }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) { return left.tunnel_id < right.tunnel_id; });
    return result;
}

std::string TunnelAllocator::report() const {
    std::ostringstream output;
    output << "tunnels.active=" << leases_.size() << ";";
    for (const auto& lease : active()) output << lease.tunnel_id << ":peer=" << lease.peer << ":g=" << lease.generation << ";";
    return output.str();
}

void TunnelAllocator::clear() { next_id_ = 1; leases_.clear(); }

}  // namespace aegis::detail
