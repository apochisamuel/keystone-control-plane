#include "internal.hpp"

namespace aegis::detail {

void AdmissionController::configure(std::uint32_t zone, std::size_t limit) { limits_[zone] = limit; }

AdmissionDecision AdmissionController::admit(std::uint32_t session_id, std::uint32_t zone) {
    if (const auto existing = sessions_.find(session_id); existing != sessions_.end()) {
        return {existing->second == zone, existing->second, existing->second == zone ? "already admitted" : "assigned elsewhere"};
    }
    const auto limit = limits_.contains(zone) ? limits_.at(zone) : 0U;
    if (limit == 0U) return {false, zone, "zone disabled"};
    if (active(zone) >= limit) return {false, zone, "zone full"};
    sessions_[session_id] = zone;
    return {true, zone, "admitted"};
}

bool AdmissionController::release(std::uint32_t session_id) { return sessions_.erase(session_id) != 0U; }

std::size_t AdmissionController::active(std::uint32_t zone) const {
    return static_cast<std::size_t>(std::count_if(sessions_.begin(), sessions_.end(), [zone](const auto& item) { return item.second == zone; }));
}

void AdmissionController::clear() { sessions_.clear(); limits_.clear(); }

}  // namespace aegis::detail
