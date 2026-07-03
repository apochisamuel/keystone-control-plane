#include "internal.hpp"

namespace aegis::detail {

std::string ScenarioTimelineBuilder::phase_name(Operation operation) const {
    switch (operation) {
        case Operation::Open: return "open";
        case Operation::Register: return "registered";
        case Operation::StagePolicy: return "staged";
        case Operation::Commit: return "active";
        case Operation::Data: return "data";
        case Operation::Handover: return "moving";
        case Operation::Rollback: return "rolled-back";
        case Operation::Close: return "closed";
        case Operation::Topology: return "topology";
        case Operation::Audit: return "audit";
        case Operation::Snapshot: return "snapshot";
        case Operation::Recover: return "recovered";
        case Operation::Extensions: return "extensions";
        case Operation::Heartbeat: return "heartbeat";
    }
    return "unknown";
}

std::vector<ScenarioStep> ScenarioTimelineBuilder::build(const std::vector<Frame>& frames) const {
    std::unordered_map<std::uint32_t, std::string> phases;
    std::vector<ScenarioStep> steps;
    steps.reserve(frames.size());
    for (std::size_t index = 0; index < frames.size(); ++index) {
        const auto& frame = frames[index];
        auto before = phases.contains(frame.session_id) ? phases.at(frame.session_id) : "absent";
        auto after = before;
        bool accepted = true;
        std::vector<std::string> notes;
        const auto payload = field_bytes(frame, FieldTag::Payload);
        std::uint64_t units = payload.size();
        switch (frame.operation) {
            case Operation::Open:
                if (before != "absent" && before != "closed") { accepted = false; notes.push_back("duplicate open"); }
                else after = "open";
                break;
            case Operation::Register:
                if (before != "open" && before != "registered") { accepted = false; notes.push_back("register requires open"); }
                else after = "registered";
                break;
            case Operation::StagePolicy:
            case Operation::Extensions:
                if (before == "absent" || before == "closed") { accepted = false; notes.push_back("stage requires session"); }
                else after = "staged";
                break;
            case Operation::Commit:
                if (before != "staged") { accepted = false; notes.push_back("commit requires staged policy"); }
                else after = "active";
                break;
            case Operation::Data:
                if (before == "absent" || before == "closed") { accepted = false; notes.push_back("data outside session"); }
                else { after = before; units = std::max<std::uint64_t>(units, 1U); }
                break;
            case Operation::Handover:
                if (before == "absent" || before == "closed") { accepted = false; notes.push_back("handover outside session"); }
                else after = "moving";
                break;
            case Operation::Rollback:
                if (before != "staged") { accepted = false; notes.push_back("rollback requires staged policy"); }
                else after = "active";
                break;
            case Operation::Close:
                if (before == "absent" || before == "closed") { accepted = false; notes.push_back("close without live session"); }
                else after = "closed";
                break;
            case Operation::Recover:
                if (before == "absent") { accepted = false; notes.push_back("recover requires retained session"); }
                else after = "active";
                break;
            case Operation::Topology:
            case Operation::Audit:
            case Operation::Snapshot:
            case Operation::Heartbeat:
                if (before == "absent" && frame.operation != Operation::Heartbeat) notes.push_back("global operation");
                break;
        }
        if (accepted) phases[frame.session_id] = after;
        steps.push_back({index, frame.session_id, frame.operation, before, after, units, accepted, std::move(notes)});
    }
    return steps;
}

}  // namespace aegis::detail
