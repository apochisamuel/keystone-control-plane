#include "internal.hpp"

namespace aegis::detail {

std::vector<ScenarioCost> ScenarioCostModel::estimate(const std::vector<ScenarioStep>& steps) const {
    std::unordered_map<std::uint32_t, ScenarioCost> costs;
    for (const auto& step : steps) {
        if (!step.accepted) continue;
        auto& value = costs[step.session_id];
        value.session_id = step.session_id;
        if (step.operation == Operation::Data) value.data_units += step.estimated_units;
        else value.control_units += 1U + static_cast<std::uint64_t>(step.notes.size());
    }
    std::vector<ScenarioCost> output;
    for (auto& [_, cost] : costs) { cost.total = cost.control_units + cost.data_units; output.push_back(cost); }
    std::sort(output.begin(), output.end(), [](const auto& left, const auto& right) { return left.session_id < right.session_id; });
    return output;
}

std::uint64_t ScenarioCostModel::aggregate(const std::vector<ScenarioCost>& costs) const {
    std::uint64_t total{};
    for (const auto& cost : costs) total += cost.total;
    return total;
}

}  // namespace aegis::detail
