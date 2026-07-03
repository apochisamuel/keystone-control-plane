#include "internal.hpp"

namespace aegis::detail {

void TunnelStore::bind(Tunnel tunnel) {
    tunnel.endpoint = canonical_text(tunnel.endpoint);
    tunnel.bound = true;
    tunnels_[tunnel.id] = std::move(tunnel);
}

std::optional<Tunnel> TunnelStore::lookup(std::uint32_t id) const {
    const auto iterator = tunnels_.find(id);
    if (iterator == tunnels_.end()) return std::nullopt;
    return iterator->second;
}

void TunnelStore::unbind(std::uint32_t id) {
    tunnels_.erase(id);
}

std::vector<Tunnel> TunnelStore::bound() const {
    std::vector<Tunnel> result;
    for (const auto& [id, tunnel] : tunnels_) {
        (void)id;
        if (tunnel.bound) result.push_back(tunnel);
    }
    std::sort(result.begin(), result.end(), [](const Tunnel& left, const Tunnel& right) {
        if (left.peer != right.peer) return left.peer < right.peer;
        return left.id < right.id;
    });
    return result;
}

std::size_t TunnelStore::size() const {
    return tunnels_.size();
}

}  // namespace aegis::detail
