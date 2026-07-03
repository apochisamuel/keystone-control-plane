#include "internal.hpp"

namespace aegis::detail {

std::optional<IdentityLease> IdentityAllocator::allocate(std::string namespace_name,
                                                         std::uint64_t generation,
                                                         std::uint64_t ttl) {
    ++tick_;
    namespace_name = canonical_text(namespace_name);
    if (namespace_name.empty() || ttl == 0U || next_id_ == std::numeric_limits<std::uint32_t>::max()) return std::nullopt;
    IdentityLease lease;
    lease.id = next_id_++;
    lease.namespace_name = std::move(namespace_name);
    lease.generation = generation;
    lease.expires = tick_ > std::numeric_limits<std::uint64_t>::max() - ttl ? std::numeric_limits<std::uint64_t>::max() : tick_ + ttl;
    leases_.emplace(lease.id, lease);
    return lease;
}

bool IdentityAllocator::release(std::uint32_t id) {
    const auto iterator = leases_.find(id);
    if (iterator == leases_.end() || iterator->second.released) return false;
    iterator->second.released = true;
    return true;
}

std::optional<IdentityLease> IdentityAllocator::lookup(std::uint32_t id) const {
    const auto iterator = leases_.find(id);
    if (iterator == leases_.end() || iterator->second.released || iterator->second.expires < tick_) return std::nullopt;
    return iterator->second;
}

std::size_t IdentityAllocator::expire(std::uint64_t advance) {
    tick_ += advance;
    const auto before = leases_.size();
    for (auto iterator = leases_.begin(); iterator != leases_.end();) {
        if (iterator->second.released || iterator->second.expires < tick_) iterator = leases_.erase(iterator);
        else ++iterator;
    }
    return before - leases_.size();
}

std::string IdentityAllocator::report() const {
    std::size_t active = 0;
    for (const auto& [id, lease] : leases_) { (void)id; if (!lease.released && lease.expires >= tick_) ++active; }
    return "identities.active=" + std::to_string(active) + ";total=" + std::to_string(leases_.size()) + ";";
}

void IdentityAllocator::clear() { next_id_ = 1000; tick_ = 0; leases_.clear(); }

}  // namespace aegis::detail
