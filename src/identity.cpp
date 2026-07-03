#include "internal.hpp"

namespace aegis::detail {

void IdentityMap::put(SubscriberProfile profile) {
    profile.id.normalized = canonical_text(profile.id.normalized);
    profile.display_name = canonical_text(profile.display_name);
    profiles_[profile.id.value] = std::move(profile);
}

std::optional<SubscriberProfile> IdentityMap::get(std::uint32_t id) const {
    const auto iterator = profiles_.find(id);
    if (iterator == profiles_.end()) return std::nullopt;
    return iterator->second;
}

void IdentityMap::erase(std::uint32_t id) {
    profiles_.erase(id);
}

std::size_t IdentityMap::size() const {
    return profiles_.size();
}

std::vector<SubscriberProfile> IdentityMap::all() const {
    std::vector<SubscriberProfile> result;
    result.reserve(profiles_.size());
    for (const auto& [id, profile] : profiles_) {
        (void)id;
        result.push_back(profile);
    }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
        return left.id.value < right.id.value;
    });
    return result;
}

}  // namespace aegis::detail
