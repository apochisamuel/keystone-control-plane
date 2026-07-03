#include "internal.hpp"

namespace aegis::detail {

std::uint64_t MaintenancePlanner::schedule(std::string component, std::string action, std::uint64_t due,
                                           std::uint64_t interval) {
    MaintenanceTask task;
    task.id = next_id_++;
    task.component = canonical_text(component.empty() ? "engine" : component);
    task.action = canonical_text(action.empty() ? "sweep" : action);
    task.due = due;
    task.interval = interval;
    task.recurring = interval != 0U;
    tasks_.push_back(std::move(task));
    return tasks_.back().id;
}

std::vector<MaintenanceTask> MaintenancePlanner::due(std::uint64_t tick, std::size_t limit) {
    std::vector<std::size_t> indexes;
    for (std::size_t index = 0; index < tasks_.size(); ++index) if (!tasks_[index].cancelled && tasks_[index].due <= tick) indexes.push_back(index);
    std::sort(indexes.begin(), indexes.end(), [this](const auto left, const auto right) {
        if (tasks_[left].due != tasks_[right].due) return tasks_[left].due < tasks_[right].due;
        return tasks_[left].id < tasks_[right].id;
    });
    std::vector<MaintenanceTask> result;
    for (const auto index : indexes) {
        if (result.size() >= limit) break;
        result.push_back(tasks_[index]);
        if (tasks_[index].recurring) tasks_[index].due = tick + tasks_[index].interval;
        else tasks_[index].cancelled = true;
    }
    return result;
}

bool MaintenancePlanner::cancel(std::uint64_t id) {
    for (auto& task : tasks_) if (task.id == id && !task.cancelled) { task.cancelled = true; return true; }
    return false;
}

std::vector<MaintenanceTask> MaintenancePlanner::pending() const {
    std::vector<MaintenanceTask> result;
    for (const auto& task : tasks_) if (!task.cancelled) result.push_back(task);
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) { return left.id < right.id; });
    return result;
}

std::string MaintenancePlanner::report() const {
    std::ostringstream output;
    output << "maintenance.pending=" << pending().size() << ";total=" << tasks_.size() << ";";
    return output.str();
}

void MaintenancePlanner::clear() { next_id_ = 1; tasks_.clear(); }

}  // namespace aegis::detail
