#include "internal.hpp"

namespace aegis::detail {

void CorrelationRegistry::link(std::string key, std::uint32_t session_id) {
    auto& rows = links_[std::move(key)];
    if (std::find(rows.begin(), rows.end(), session_id) == rows.end()) rows.push_back(session_id);
}

std::vector<std::uint32_t> CorrelationRegistry::lookup(std::string_view key) const {
    const auto found = links_.find(std::string(key));
    return found == links_.end() ? std::vector<std::uint32_t>{} : found->second;
}

std::size_t CorrelationRegistry::erase(std::uint32_t session_id) {
    std::size_t removed{};
    for (auto it = links_.begin(); it != links_.end();) {
        const auto before = it->second.size();
        it->second.erase(std::remove(it->second.begin(), it->second.end(), session_id), it->second.end());
        removed += before - it->second.size();
        it = it->second.empty() ? links_.erase(it) : std::next(it);
    }
    return removed;
}

std::string CorrelationRegistry::report() const {
    std::vector<std::vector<std::string>> rows{{"key", "sessions"}};
    for (const auto& [key, values] : links_) { std::vector<std::string> ids; for (auto id : values) ids.push_back(std::to_string(id)); rows.push_back({key, join_labels(ids, ",")}); }
    return render_table(rows);
}

void CorrelationRegistry::clear() { links_.clear(); }

}  // namespace aegis::detail
