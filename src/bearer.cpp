#include "internal.hpp"

namespace aegis::detail {

void BearerStore::install(Bearer bearer) {
    bearer.label = canonical_text(bearer.label);
    bearers_[bearer.id] = std::move(bearer);
}

std::optional<Bearer> BearerStore::find(std::uint32_t id) const {
    const auto iterator = bearers_.find(id);
    if (iterator == bearers_.end()) return std::nullopt;
    return iterator->second;
}

void BearerStore::remove(std::uint32_t id) {
    bearers_.erase(id);
}

std::vector<Bearer> BearerStore::active() const {
    std::vector<Bearer> result;
    for (const auto& [id, bearer] : bearers_) {
        (void)id;
        if (bearer.active) result.push_back(bearer);
    }
    std::sort(result.begin(), result.end(), [](const Bearer& left, const Bearer& right) {
        return left.id < right.id;
    });
    return result;
}

std::size_t BearerStore::size() const {
    return bearers_.size();
}

}  // namespace aegis::detail
