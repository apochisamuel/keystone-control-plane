#include "internal.hpp"

namespace aegis::detail {

SimulationValidation SimulationValidator::validate(const SimulationRun& run, const SimulationParameters& parameters) const {
    SimulationValidation validation;
    validation.valid = true;
    if (run.frames.empty()) {
        validation.valid = false;
        validation.errors.push_back("simulation emitted no frames");
        return validation;
    }
    if (run.frames.front().operation != Operation::Open) {
        validation.valid = false;
        validation.errors.push_back("first frame is not open");
    }
    if (run.frames.back().operation != Operation::Close) {
        validation.valid = false;
        validation.errors.push_back("last frame is not close");
    }
    std::uint32_t expected_sequence = 1U;
    std::size_t data_frames{};
    std::uint64_t payload_total{};
    bool saw_commit{};
    for (std::size_t index = 0; index < run.frames.size(); ++index) {
        const auto& frame = run.frames[index];
        if (frame.session_id != parameters.session_id) {
            validation.valid = false;
            validation.errors.push_back("session mismatch at frame " + std::to_string(index + 1U));
        }
        const auto sequence = field_u32(frame, FieldTag::Sequence, 0U);
        if (sequence != expected_sequence) {
            validation.valid = false;
            validation.errors.push_back("sequence discontinuity at frame " + std::to_string(index + 1U));
            expected_sequence = sequence;
        }
        ++expected_sequence;
        if (frame.operation == Operation::Commit) saw_commit = true;
        if (frame.operation == Operation::Data) {
            ++data_frames;
            if (!saw_commit) {
                validation.valid = false;
                validation.errors.push_back("data appears before commit");
            }
            const auto payload = field_bytes(frame, FieldTag::Payload);
            payload_total += payload.size();
            if (payload.size() < parameters.minimum_payload || payload.size() > parameters.maximum_payload) {
                validation.valid = false;
                validation.errors.push_back("payload outside configured range");
            }
        }
        if (frame.operation == Operation::Handover && data_frames == 0U) validation.warnings.push_back("handover without data");
    }
    if (data_frames != std::max(1U, parameters.rounds)) {
        validation.valid = false;
        validation.errors.push_back("unexpected data-frame count");
    }
    if (payload_total != run.generated_payload - 11U) {
        validation.warnings.push_back("control-unit accounting differs from payload ledger");
    }
    const auto encoded = write_capture(run.frames);
    std::vector<std::string> errors;
    const auto decoded = parse_capture(encoded, errors);
    if (!errors.empty() || decoded.size() != run.frames.size()) {
        validation.valid = false;
        validation.errors.push_back("generated capture does not round-trip");
    }
    return validation;
}

std::string SimulationValidator::render(const SimulationValidation& validation) const {
    std::vector<std::vector<std::string>> rows{{"class", "message"}};
    for (const auto& error : validation.errors) rows.push_back({"error", error});
    for (const auto& warning : validation.warnings) rows.push_back({"warning", warning});
    if (validation.errors.empty() && validation.warnings.empty()) rows.push_back({"status", "clean"});
    return render_table(rows);
}

}  // namespace aegis::detail
