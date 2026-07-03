#include "internal.hpp"

namespace aegis::detail {

std::string ScenarioReportRenderer::render_steps(const std::vector<ScenarioStep>& steps) const {
    std::vector<std::vector<std::string>> rows{{"index", "session", "operation", "before", "after", "units", "accepted", "notes"}};
    for (const auto& step : steps) rows.push_back({std::to_string(step.index), std::to_string(step.session_id), describe_operation(step.operation), step.phase_before, step.phase_after, std::to_string(step.estimated_units), step.accepted ? "yes" : "no", join_labels(step.notes, ";")});
    return render_table(rows);
}

std::string ScenarioReportRenderer::render_costs(const std::vector<ScenarioCost>& costs) const {
    std::vector<std::vector<std::string>> rows{{"session", "control", "data", "total"}};
    for (const auto& cost : costs) rows.push_back({std::to_string(cost.session_id), std::to_string(cost.control_units), std::to_string(cost.data_units), std::to_string(cost.total)});
    return render_table(rows);
}

std::string ScenarioReportRenderer::render_findings(const std::vector<std::string>& findings) const {
    std::vector<std::vector<std::string>> rows{{"finding"}};
    for (const auto& finding : findings) rows.push_back({finding});
    return render_table(rows);
}

}  // namespace aegis::detail
