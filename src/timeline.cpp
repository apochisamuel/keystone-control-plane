#include "internal.hpp"

namespace aegis::detail {

void SessionTimeline::append(std::uint32_t session_id, std::uint64_t generation, std::string kind, std::string detail) {
    TimelineEvent event;
    event.sequence = next_sequence_++;
    event.session_id = session_id;
    event.generation = generation;
    event.kind = canonical_text(kind.empty() ? "event" : kind);
    event.detail = std::move(detail);
    entries_.push_back(std::move(event));
}

std::vector<TimelineEvent> SessionTimeline::events(std::uint32_t session_id) const {
    std::vector<TimelineEvent> result;
    for (const auto& entry : entries_) if (entry.session_id == session_id) result.push_back(entry);
    return result;
}

std::vector<TimelineEvent> SessionTimeline::between(std::uint64_t first, std::uint64_t last) const {
    if (first > last) std::swap(first, last);
    std::vector<TimelineEvent> result;
    for (const auto& entry : entries_) if (entry.sequence >= first && entry.sequence <= last) result.push_back(entry);
    return result;
}

std::string SessionTimeline::render(std::uint32_t session_id) const {
    std::ostringstream output;
    for (const auto& event : events(session_id)) output << event.sequence << ":g=" << event.generation << ":" << event.kind << ":" << event.detail << "\n";
    return output.str();
}

void SessionTimeline::clear() { next_sequence_ = 1; entries_.clear(); }

}  // namespace aegis::detail
