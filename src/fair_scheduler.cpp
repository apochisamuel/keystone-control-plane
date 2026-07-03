#include "internal.hpp"

namespace aegis::detail {

void FairScheduler::register_session(std::uint32_t session_id, std::uint32_t priority) {
    auto& slot = slots_[session_id];
    slot.session_id = session_id;
    slot.priority = std::clamp(priority, 1U, 100U);
}

void FairScheduler::charge(std::uint32_t session_id, std::uint64_t units) {
    auto& slot = slots_[session_id];
    slot.session_id = session_id;
    if (slot.priority == 0U) slot.priority = 1U;
    const auto normalized = units / slot.priority + 1U;
    slot.debt = std::min<std::uint64_t>(std::numeric_limits<std::uint64_t>::max() - normalized, slot.debt) + normalized;
    slot.served++;
}

std::optional<std::uint32_t> FairScheduler::next() const {
    if (slots_.empty()) return std::nullopt;
    const auto iterator = std::min_element(slots_.begin(), slots_.end(), [](const auto& left, const auto& right) {
        const auto left_score = left.second.debt / std::max(1U, left.second.priority);
        const auto right_score = right.second.debt / std::max(1U, right.second.priority);
        if (left_score != right_score) return left_score < right_score;
        return left.first < right.first;
    });
    return iterator->first;
}

std::vector<FairnessSlot> FairScheduler::slots() const {
    std::vector<FairnessSlot> result;
    for (const auto& [id, slot] : slots_) { (void)id; result.push_back(slot); }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) { return left.session_id < right.session_id; });
    return result;
}

std::string FairScheduler::report() const {
    std::ostringstream output;
    output << "fair.sessions=" << slots_.size() << ";";
    for (const auto& slot : slots()) output << slot.session_id << ":debt=" << slot.debt << ":served=" << slot.served << ";";
    return output.str();
}

void FairScheduler::clear() { slots_.clear(); }

}  // namespace aegis::detail
