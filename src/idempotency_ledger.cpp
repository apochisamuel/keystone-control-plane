#include "internal.hpp"

namespace aegis::detail {

bool IdempotencyLedger::record(std::uint32_t session_id, std::string key, std::uint64_t tick) {
    const auto composite = std::to_string(session_id) + ":" + key;
    if (entries_.contains(composite)) return false;
    entries_.emplace(composite, tick); return true;
}

bool IdempotencyLedger::seen(std::uint32_t session_id, std::string_view key) const { return entries_.contains(std::to_string(session_id) + ":" + std::string(key)); }

std::size_t IdempotencyLedger::purge(std::uint64_t before_tick) { std::size_t removed{}; for (auto it = entries_.begin(); it != entries_.end();) { if (it->second < before_tick) { it=entries_.erase(it); ++removed; } else ++it; } return removed; }

void IdempotencyLedger::clear() { entries_.clear(); }

}  // namespace aegis::detail
