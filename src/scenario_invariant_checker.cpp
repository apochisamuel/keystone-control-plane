#include "internal.hpp"

namespace aegis::detail {

std::vector<std::string> ScenarioInvariantChecker::validate(const std::vector<ScenarioStep>& steps) const {
    std::vector<std::string> findings;
    std::unordered_map<std::uint32_t, std::string> phase;
    std::unordered_map<std::uint32_t, std::uint64_t> data_units;
    for (const auto& step : steps) {
        const auto current = phase.contains(step.session_id) ? phase.at(step.session_id) : "absent";
        if (current != step.phase_before) findings.push_back("step " + std::to_string(step.index) + " has discontinuous phase");
        if (!step.accepted) findings.push_back("step " + std::to_string(step.index) + " rejected: " + describe_operation(step.operation));
        if (step.operation == Operation::Data && step.accepted) data_units[step.session_id] += step.estimated_units;
        if (step.operation == Operation::Close && step.accepted && data_units[step.session_id] == 0U) findings.push_back("session " + std::to_string(step.session_id) + " closed without data");
        if (step.accepted) phase[step.session_id] = step.phase_after;
    }
    for (const auto& [session, state] : phase) if (state == "moving") findings.push_back("session " + std::to_string(session) + " remains moving");
    return findings;
}

std::map<std::uint32_t, std::string> ScenarioInvariantChecker::final_phases(const std::vector<ScenarioStep>& steps) const {
    std::map<std::uint32_t, std::string> phases;
    for (const auto& step : steps) if (step.accepted) phases[step.session_id] = step.phase_after;
    return phases;
}

}  // namespace aegis::detail
