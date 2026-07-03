#include "internal.hpp"

namespace aegis::detail {

std::uint64_t PriorityScheduler::enqueue(std::uint32_t session_id, std::uint32_t priority,
                                         std::uint64_t delay, std::uint64_t generation,
                                         std::string kind, std::string payload) {
    WorkItem item;
    item.id = next_id_++;
    item.session_id = session_id;
    item.priority = std::min(priority, 100U);
    item.due_tick = delay;
    item.generation = generation;
    item.kind = canonical_text(kind.empty() ? "data" : kind);
    item.payload = std::move(payload);
    items_.push_back(std::move(item));
    return items_.back().id;
}

std::vector<WorkItem> PriorityScheduler::take_ready(std::uint64_t tick, std::size_t limit) {
    std::vector<std::size_t> ready_indices;
    for (std::size_t index = 0; index < items_.size(); ++index) {
        const auto& item = items_[index];
        if (!item.cancelled && item.due_tick <= tick) ready_indices.push_back(index);
    }
    std::sort(ready_indices.begin(), ready_indices.end(), [this](std::size_t left, std::size_t right) {
        const auto& a = items_[left];
        const auto& b = items_[right];
        if (a.priority != b.priority) return a.priority > b.priority;
        if (a.due_tick != b.due_tick) return a.due_tick < b.due_tick;
        return a.id < b.id;
    });
    std::vector<WorkItem> result;
    for (const auto index : ready_indices) {
        if (result.size() >= limit) break;
        result.push_back(items_[index]);
        items_[index].cancelled = true;
    }
    return result;
}

bool PriorityScheduler::cancel(std::uint64_t id) {
    for (auto& item : items_) {
        if (item.id != id || item.cancelled) continue;
        item.cancelled = true;
        return true;
    }
    return false;
}

std::size_t PriorityScheduler::cancel_session(std::uint32_t session_id) {
    std::size_t count = 0;
    for (auto& item : items_) {
        if (item.session_id == session_id && !item.cancelled) { item.cancelled = true; ++count; }
    }
    return count;
}

void PriorityScheduler::rebalance(std::uint32_t session_id, std::uint32_t floor, std::uint32_t ceiling) {
    if (floor > ceiling) std::swap(floor, ceiling);
    ceiling = std::min(ceiling, 100U);
    for (auto& item : items_) {
        if (item.session_id != session_id || item.cancelled) continue;
        item.priority = std::clamp(item.priority, floor, ceiling);
    }
}

std::vector<WorkItem> PriorityScheduler::pending(std::uint32_t session_id) const {
    std::vector<WorkItem> result;
    for (const auto& item : items_) if (item.session_id == session_id && !item.cancelled) result.push_back(item);
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
        if (left.due_tick != right.due_tick) return left.due_tick < right.due_tick;
        return left.id < right.id;
    });
    return result;
}

std::string PriorityScheduler::report() const {
    std::size_t active = 0;
    for (const auto& item : items_) if (!item.cancelled) ++active;
    std::ostringstream output;
    output << "scheduler.total=" << items_.size() << ";active=" << active << ";";
    return output.str();
}

void PriorityScheduler::clear() {
    next_id_ = 1;
    items_.clear();
}

}  // namespace aegis::detail
