#include "internal.hpp"

namespace aegis::detail {

void RetentionStore::put(std::string key, std::string category, std::string value, std::uint64_t ttl) {
    ++tick_;
    key = canonical_text(key);
    if (key.empty()) return;
    RetentionItem item;
    item.key = std::move(key);
    item.category = canonical_text(category.empty() ? "general" : category);
    item.value = std::move(value);
    item.created = tick_;
    item.expires = ttl > std::numeric_limits<std::uint64_t>::max() - tick_ ? std::numeric_limits<std::uint64_t>::max() : tick_ + ttl;
    items_[item.key] = std::move(item);
}

std::optional<RetentionItem> RetentionStore::get(std::string_view key) const {
    const auto iterator = items_.find(canonical_text(key));
    if (iterator == items_.end() || iterator->second.expires < tick_) return std::nullopt;
    return iterator->second;
}

std::vector<RetentionItem> RetentionStore::find(std::string_view category) const {
    const auto expected = canonical_text(category);
    std::vector<RetentionItem> result;
    for (const auto& [key, item] : items_) {
        (void)key;
        if (item.category == expected && item.expires >= tick_) result.push_back(item);
    }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) { return left.key < right.key; });
    return result;
}

std::size_t RetentionStore::expire(std::uint64_t advance) {
    tick_ += advance;
    const auto before = items_.size();
    for (auto iterator = items_.begin(); iterator != items_.end();) {
        if (iterator->second.expires < tick_) iterator = items_.erase(iterator);
        else ++iterator;
    }
    return before - items_.size();
}

std::size_t RetentionStore::size() const { return items_.size(); }

std::string RetentionStore::report() const {
    std::unordered_map<std::string, std::size_t> count;
    for (const auto& [key, item] : items_) { (void)key; count[item.category]++; }
    std::vector<std::string> categories;
    for (const auto& [name, value] : count) { (void)value; categories.push_back(name); }
    std::sort(categories.begin(), categories.end());
    std::ostringstream output;
    output << "retention.items=" << items_.size() << ";tick=" << tick_ << ";";
    for (const auto& category : categories) output << category << "=" << count.at(category) << ";";
    return output.str();
}

void RetentionStore::clear() { tick_ = 0; items_.clear(); }

}  // namespace aegis::detail
