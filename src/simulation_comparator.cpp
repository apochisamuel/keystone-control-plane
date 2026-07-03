#include "internal.hpp"

namespace aegis::detail {

namespace {
std::map<std::string, std::pair<std::int64_t, std::int64_t>> summarize(const SimulationRun& run) {
    std::map<std::string, std::pair<std::int64_t, std::int64_t>> totals;
    for (const auto& metric : run.metrics) {
        auto& value = totals[metric.category];
        ++value.first;
        value.second += static_cast<std::int64_t>(metric.units);
    }
    return totals;
}
}

std::vector<SimulationDelta> SimulationComparator::compare(const SimulationRun& baseline, const SimulationRun& candidate) const {
    const auto left = summarize(baseline);
    const auto right = summarize(candidate);
    std::set<std::string> categories;
    for (const auto& [category, _] : left) categories.insert(category);
    for (const auto& [category, _] : right) categories.insert(category);
    std::vector<SimulationDelta> deltas;
    for (const auto& category : categories) {
        const auto before = left.contains(category) ? left.at(category) : std::pair<std::int64_t, std::int64_t>{};
        const auto after = right.contains(category) ? right.at(category) : std::pair<std::int64_t, std::int64_t>{};
        const auto frames = after.first - before.first;
        const auto units = after.second - before.second;
        if (frames == 0 && units == 0) continue;
        std::string explanation;
        if (frames > 0) explanation = "additional frames";
        else if (frames < 0) explanation = "fewer frames";
        else if (units > 0) explanation = "higher unit volume";
        else explanation = "lower unit volume";
        deltas.push_back({category, frames, units, std::move(explanation)});
    }
    return deltas;
}

std::string SimulationComparator::render(const std::vector<SimulationDelta>& deltas) const {
    std::vector<std::vector<std::string>> rows{{"category", "frame_delta", "unit_delta", "explanation"}};
    for (const auto& delta : deltas) rows.push_back({delta.category, std::to_string(delta.frame_delta), std::to_string(delta.unit_delta), delta.explanation});
    return render_table(rows);
}

bool SimulationComparator::equivalent(const SimulationRun& left, const SimulationRun& right) const {
    if (left.frames.size() != right.frames.size() || left.generated_payload != right.generated_payload) return false;
    const auto deltas = compare(left, right);
    if (!deltas.empty()) return false;
    for (std::size_t index = 0; index < left.frames.size(); ++index) {
        const auto left_bytes = write_frame(left.frames[index]);
        const auto right_bytes = write_frame(right.frames[index]);
        if (left_bytes != right_bytes) return false;
    }
    return true;
}

}  // namespace aegis::detail
