#include "internal.hpp"

namespace aegis::detail {

std::uint64_t CheckpointManager::capture(std::uint32_t session_id, std::uint64_t generation,
                                         std::string reason, std::string state) {
    Checkpoint point;
    point.id = next_id_++;
    point.session_id = session_id;
    point.generation = generation;
    point.reason = canonical_text(reason.empty() ? "periodic" : reason);
    point.state = std::move(state);
    point.digest = stable_digest(std::to_string(session_id) + "|" + std::to_string(generation) + "|" + point.state);
    point.captured_tick = ++tick_;
    points_.push_back(std::move(point));
    return points_.back().id;
}

std::optional<Checkpoint> CheckpointManager::latest(std::uint32_t session_id) const {
    for (auto iterator = points_.rbegin(); iterator != points_.rend(); ++iterator) {
        if (iterator->session_id == session_id) return *iterator;
    }
    return std::nullopt;
}

std::vector<Checkpoint> CheckpointManager::history(std::uint32_t session_id) const {
    std::vector<Checkpoint> result;
    for (const auto& point : points_) if (point.session_id == session_id) result.push_back(point);
    return result;
}

std::vector<Checkpoint> CheckpointManager::all() const { return points_; }

bool CheckpointManager::verify(std::uint64_t id) const {
    const auto iterator = std::find_if(points_.begin(), points_.end(), [id](const Checkpoint& point) { return point.id == id; });
    if (iterator == points_.end()) return false;
    return iterator->digest == stable_digest(std::to_string(iterator->session_id) + "|" +
        std::to_string(iterator->generation) + "|" + iterator->state);
}

std::size_t CheckpointManager::prune(std::uint32_t session_id, std::size_t limit) {
    auto matching = history(session_id);
    if (matching.size() <= limit) return 0U;
    std::sort(matching.begin(), matching.end(), [](const auto& left, const auto& right) { return left.id < right.id; });
    std::unordered_set<std::uint64_t> keep;
    for (std::size_t index = matching.size() - limit; index < matching.size(); ++index) keep.insert(matching[index].id);
    const auto before = points_.size();
    points_.erase(std::remove_if(points_.begin(), points_.end(), [&](const Checkpoint& point) {
        return point.session_id == session_id && !keep.contains(point.id);
    }), points_.end());
    return before - points_.size();
}

std::string CheckpointManager::export_session(std::uint32_t session_id) const {
    std::ostringstream output;
    for (const auto& point : history(session_id)) {
        output << point.id << ":" << point.generation << ":" << point.reason << ":" << point.digest << "\n";
    }
    return output.str();
}

std::string CheckpointManager::report() const {
    std::ostringstream output;
    output << "checkpoints=" << points_.size() << ";";
    return output.str();
}

void CheckpointManager::clear() {
    next_id_ = 1;
    tick_ = 0;
    points_.clear();
}

}  // namespace aegis::detail
