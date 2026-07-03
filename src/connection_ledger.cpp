#include "internal.hpp"

namespace aegis::detail {

void ConnectionLedger::open(std::uint32_t session_id, std::string endpoint, std::uint64_t tick) {
    auto& entry = entries_[session_id];
    entry.endpoint = std::move(endpoint);
    entry.opened = tick;
    entry.closed = 0U;
    entry.live = true;
}

void ConnectionLedger::close(std::uint32_t session_id, std::uint64_t tick) {
    if (auto found = entries_.find(session_id); found != entries_.end()) { found->second.live = false; found->second.closed = tick; }
}

std::optional<std::string> ConnectionLedger::endpoint(std::uint32_t session_id) const {
    const auto found = entries_.find(session_id);
    if (found == entries_.end() || !found->second.live) return std::nullopt;
    return found->second.endpoint;
}

std::size_t ConnectionLedger::active() const {
    return static_cast<std::size_t>(std::count_if(entries_.begin(), entries_.end(), [](const auto& item) { return item.second.live; }));
}

std::string ConnectionLedger::report() const {
    std::vector<std::vector<std::string>> rows{{"session", "endpoint", "opened", "closed", "state"}};
    for (const auto& [id, entry] : entries_) rows.push_back({std::to_string(id), entry.endpoint, std::to_string(entry.opened), std::to_string(entry.closed), entry.live ? "open" : "closed"});
    return render_table(rows);
}

void ConnectionLedger::clear() { entries_.clear(); }

}  // namespace aegis::detail
