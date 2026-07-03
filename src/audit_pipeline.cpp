#include "internal.hpp"

namespace aegis::detail {

std::uint64_t AuditPipeline::submit(std::uint32_t session_id, std::string actor, std::string action,
                                    std::string target, bool redact) {
    AuditEnvelope event;
    event.id = next_id_++;
    event.session_id = session_id;
    event.actor = canonical_text(actor.empty() ? "system" : actor);
    event.action = canonical_text(action.empty() ? "observe" : action);
    event.target = redact ? "[redacted]" : std::move(target);
    event.redacted = redact;
    event.fingerprint = stable_digest(event.actor + "|" + event.action + "|" + event.target);
    events_.push_back(std::move(event));
    return events_.back().id;
}

std::vector<AuditEnvelope> AuditPipeline::search(std::uint32_t session_id, std::string_view action) const {
    const auto expected = canonical_text(action);
    std::vector<AuditEnvelope> result;
    for (const auto& event : events_) {
        if (event.session_id != session_id) continue;
        if (!expected.empty() && event.action != expected) continue;
        result.push_back(event);
    }
    return result;
}

std::string AuditPipeline::render(std::uint32_t session_id) const {
    std::ostringstream output;
    for (const auto& event : search(session_id)) {
        output << event.id << ":" << event.actor << ":" << event.action << ":" << event.target << ":" << event.fingerprint << "\n";
    }
    return output.str();
}

std::size_t AuditPipeline::size() const { return events_.size(); }
void AuditPipeline::clear() { next_id_ = 1; events_.clear(); }

}  // namespace aegis::detail
