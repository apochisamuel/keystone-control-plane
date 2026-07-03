#include "internal.hpp"

namespace aegis::detail {

void DiagnosticPipeline::add(DiagnosticObservation observation) {
    observations_.push_back(std::move(observation));
    if (observations_.size() > 2048U) observations_.erase(observations_.begin(), observations_.begin() + 512U);
}

void DiagnosticPipeline::observe(std::uint64_t ordinal, const Frame& frame, std::span<const Byte> encoded) {
    const auto payload = field_bytes(frame, FieldTag::Payload);
    add({ordinal, frame.session_id, "frame", "info", describe_operation(frame.operation), encoded.size()});
    if (encoded.size() > 4096U) add({ordinal, frame.session_id, "size", "warning", "large encoded frame", encoded.size()});
    if (payload.size() > 2048U) add({ordinal, frame.session_id, "payload", "warning", "large payload", payload.size()});
    if (frame.fields.empty() && frame.operation != Operation::Heartbeat && frame.operation != Operation::Data) {
        add({ordinal, frame.session_id, "shape", "warning", "control frame lacks fields", 0U});
    }
    std::set<Byte> tags;
    for (const auto& field : frame.fields) {
        if (!tags.insert(field.tag).second) add({ordinal, frame.session_id, "shape", "warning", "duplicate field tag", field.tag});
        if (field.value.size() > 8192U) add({ordinal, frame.session_id, "field", "warning", "large field value", field.value.size()});
    }
    if (frame.operation == Operation::Data && payload.empty()) add({ordinal, frame.session_id, "payload", "info", "empty data payload", 0U});
}

std::vector<DiagnosticObservation> DiagnosticPipeline::session(std::uint32_t session_id) const {
    std::vector<DiagnosticObservation> result;
    for (const auto& observation : observations_) if (observation.session_id == session_id) result.push_back(observation);
    return result;
}

std::vector<DiagnosticObservation> DiagnosticPipeline::severity(std::string_view value) const {
    std::vector<DiagnosticObservation> result;
    for (const auto& observation : observations_) if (observation.severity == value) result.push_back(observation);
    return result;
}

std::map<std::string, std::uint64_t> DiagnosticPipeline::totals() const {
    std::map<std::string, std::uint64_t> result;
    for (const auto& observation : observations_) ++result[observation.category + "." + observation.severity];
    return result;
}

std::string DiagnosticPipeline::render(std::size_t limit) const {
    std::vector<std::vector<std::string>> rows{{"ordinal", "session", "category", "severity", "detail", "value"}};
    const auto start = observations_.size() > limit ? observations_.size() - limit : 0U;
    for (std::size_t index = start; index < observations_.size(); ++index) {
        const auto& item = observations_[index];
        rows.push_back({std::to_string(item.ordinal), std::to_string(item.session_id), item.category, item.severity, item.detail, std::to_string(item.value)});
    }
    return render_table(rows);
}

void DiagnosticPipeline::clear() { observations_.clear(); }

}  // namespace aegis::detail
