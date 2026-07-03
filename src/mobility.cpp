#include "internal.hpp"

namespace aegis::detail {

std::uint64_t MobilityTracker::begin(std::uint32_t session_id, std::uint32_t origin,
                                     std::uint32_t destination, std::uint64_t generation) {
    MobilityRecord record;
    record.id = next_id_++;
    record.session_id = session_id;
    record.origin = origin;
    record.destination = destination;
    record.generation = generation;
    record.checkpoints.push_back("prepared");
    records_.emplace(record.id, record);
    return record.id;
}

bool MobilityTracker::admit(std::uint64_t id, std::string checkpoint) {
    auto iterator = records_.find(id);
    if (iterator == records_.end() || iterator->second.phase != MobilityPhase::Prepared) return false;
    iterator->second.phase = MobilityPhase::Admitted;
    iterator->second.checkpoints.push_back(canonical_text(checkpoint.empty() ? "admitted" : checkpoint));
    return true;
}

bool MobilityTracker::transfer(std::uint64_t id, std::string checkpoint) {
    auto iterator = records_.find(id);
    if (iterator == records_.end() || iterator->second.phase != MobilityPhase::Admitted) return false;
    iterator->second.phase = MobilityPhase::Transferred;
    iterator->second.checkpoints.push_back(canonical_text(checkpoint.empty() ? "transferred" : checkpoint));
    return true;
}

bool MobilityTracker::complete(std::uint64_t id, std::string confirmation) {
    auto iterator = records_.find(id);
    if (iterator == records_.end() || iterator->second.phase != MobilityPhase::Transferred) return false;
    iterator->second.phase = MobilityPhase::Completed;
    iterator->second.checkpoints.push_back(canonical_text(confirmation.empty() ? "completed" : confirmation));
    return true;
}

bool MobilityTracker::abort(std::uint64_t id, std::string reason) {
    auto iterator = records_.find(id);
    if (iterator == records_.end()) return false;
    if (iterator->second.phase == MobilityPhase::Completed || iterator->second.phase == MobilityPhase::Aborted) return false;
    iterator->second.phase = MobilityPhase::Aborted;
    iterator->second.reason = canonical_text(reason.empty() ? "aborted" : reason);
    return true;
}

std::optional<MobilityRecord> MobilityTracker::find(std::uint64_t id) const {
    const auto iterator = records_.find(id);
    if (iterator == records_.end()) return std::nullopt;
    return iterator->second;
}

std::vector<MobilityRecord> MobilityTracker::active(std::uint32_t session_id) const {
    std::vector<MobilityRecord> result;
    for (const auto& [id, record] : records_) {
        (void)id;
        if (record.session_id != session_id) continue;
        if (record.phase != MobilityPhase::Completed && record.phase != MobilityPhase::Aborted) result.push_back(record);
    }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) { return left.id < right.id; });
    return result;
}

std::size_t MobilityTracker::close_generation(std::uint32_t session_id, std::uint64_t generation) {
    std::size_t count = 0;
    for (auto& [id, record] : records_) {
        (void)id;
        if (record.session_id == session_id && record.generation == generation &&
            record.phase != MobilityPhase::Completed && record.phase != MobilityPhase::Aborted) {
            record.phase = MobilityPhase::Aborted;
            record.reason = "generation-closed";
            ++count;
        }
    }
    return count;
}

std::string MobilityTracker::report() const {
    std::array<std::size_t, 5> counts{};
    for (const auto& [id, record] : records_) {
        (void)id;
        counts[static_cast<std::size_t>(record.phase)]++;
    }
    std::ostringstream output;
    output << "mobility.records=" << records_.size() << ";prepared=" << counts[0]
           << ";admitted=" << counts[1] << ";transferred=" << counts[2]
           << ";completed=" << counts[3] << ";aborted=" << counts[4] << ";";
    return output.str();
}

void MobilityTracker::clear() {
    next_id_ = 1;
    records_.clear();
}

}  // namespace aegis::detail
