#include "internal.hpp"

namespace aegis::detail {

void StableRouteCache::put(std::uint32_t session_id, std::uint64_t generation, std::uint32_t route_id,
                           std::string key) {
    CachedRoute entry;
    entry.session_id = session_id;
    entry.generation = generation;
    entry.route_id = route_id;
    entry.key = canonical_text(key);
    const auto prior = entries_.find(session_id);
    if (prior != entries_.end() && prior->second.generation == generation && prior->second.route_id == route_id) entry.hits = prior->second.hits;
    entries_[session_id] = std::move(entry);
}

std::optional<CachedRoute> StableRouteCache::get(std::uint32_t session_id, std::uint64_t generation) const {
    const auto iterator = entries_.find(session_id);
    if (iterator == entries_.end() || iterator->second.generation != generation) return std::nullopt;
    return iterator->second;
}

std::size_t StableRouteCache::invalidate_generation(std::uint32_t session_id, std::uint64_t keep_generation) {
    const auto iterator = entries_.find(session_id);
    if (iterator == entries_.end() || iterator->second.generation == keep_generation) return 0U;
    entries_.erase(iterator);
    return 1U;
}

std::vector<CachedRoute> StableRouteCache::entries() const {
    std::vector<CachedRoute> result;
    for (const auto& [id, entry] : entries_) { (void)id; result.push_back(entry); }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) { return left.session_id < right.session_id; });
    return result;
}

std::string StableRouteCache::report() const {
    std::ostringstream output;
    output << "stable-route-cache=" << entries_.size() << ";";
    for (const auto& entry : entries()) output << entry.session_id << ":g=" << entry.generation << ":route=" << entry.route_id << ";";
    return output.str();
}

void StableRouteCache::clear() { entries_.clear(); }

}  // namespace aegis::detail
