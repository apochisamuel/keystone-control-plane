#include "internal.hpp"

namespace aegis::detail {

std::vector<RecoveryStep> RecoveryPlanner::plan(const std::vector<JournalEntry>& entries) const {
    std::vector<RecoveryStep> steps;
    std::unordered_map<std::uint32_t, std::uint64_t> latest;
    for (const auto& entry : entries) {
        latest[entry.session_id] = std::max(latest[entry.session_id], entry.sequence);
        RecoveryStep step;
        step.sequence = entry.sequence;
        if (entry.component == "session") {
            step.component = "session";
            step.action = "restore";
        } else if (entry.component == "policy") {
            step.component = "policy";
            step.action = "activate";
            step.dependencies.push_back("session:restore");
        } else if (entry.component == "mobility") {
            step.component = "mobility";
            step.action = "reconcile";
            step.dependencies = {"session:restore", "policy:activate"};
        } else if (entry.component == "dispatch") {
            step.component = "dispatch";
            step.action = "rebuild";
            step.dependencies = {"session:restore", "policy:activate"};
        } else {
            step.component = entry.component;
            step.action = "replay";
        }
        steps.push_back(std::move(step));
    }
    std::stable_sort(steps.begin(), steps.end(), [](const RecoveryStep& left, const RecoveryStep& right) {
        const auto rank = [](const RecoveryStep& step) {
            if (step.component == "session") return 0;
            if (step.component == "policy") return 1;
            if (step.component == "mobility") return 2;
            if (step.component == "dispatch") return 3;
            return 4;
        };
        if (rank(left) != rank(right)) return rank(left) < rank(right);
        return left.sequence < right.sequence;
    });
    return steps;
}

std::vector<std::string> RecoveryPlanner::validate(const std::vector<RecoveryStep>& steps) const {
    std::vector<std::string> errors;
    std::set<std::string> completed;
    for (const auto& step : steps) {
        for (const auto& dependency : step.dependencies) if (!completed.contains(dependency)) errors.push_back("missing dependency " + dependency + " before " + step.component);
        completed.insert(step.component + ":" + step.action);
    }
    return errors;
}

std::string RecoveryPlanner::render(const std::vector<RecoveryStep>& steps) const {
    std::ostringstream output;
    for (const auto& step : steps) {
        output << step.sequence << ":" << step.component << ":" << step.action << ":deps=" << join_labels(step.dependencies, ",") << "\n";
    }
    return output.str();
}

}  // namespace aegis::detail
