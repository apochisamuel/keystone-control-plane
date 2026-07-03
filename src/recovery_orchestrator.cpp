#include "internal.hpp"

namespace aegis::detail {

std::vector<RecoveryAction> RecoveryOrchestrator::plan(const std::vector<RecoveryEntry>& entries,
                                                       const std::vector<Checkpoint>& checkpoints) const {
    std::unordered_map<std::uint32_t, Checkpoint> latest_checkpoint;
    for (const auto& checkpoint : checkpoints) {
        const auto found = latest_checkpoint.find(checkpoint.session_id);
        if (found == latest_checkpoint.end() || checkpoint.generation > found->second.generation ||
            (checkpoint.generation == found->second.generation && checkpoint.id > found->second.id)) {
            latest_checkpoint[checkpoint.session_id] = checkpoint;
        }
    }
    std::vector<RecoveryEntry> ordered = entries;
    std::sort(ordered.begin(), ordered.end(), [](const auto& left, const auto& right) {
        if (left.session_id != right.session_id) return left.session_id < right.session_id;
        if (left.generation != right.generation) return left.generation < right.generation;
        return left.label < right.label;
    });
    std::vector<RecoveryAction> actions;
    std::uint64_t next_sequence{1U};
    std::unordered_map<std::uint32_t, std::uint64_t> last_by_session;
    for (const auto& entry : ordered) {
        RecoveryAction action;
        action.sequence = next_sequence++;
        action.session_id = entry.session_id;
        action.generation = entry.generation;
        action.component = "recovery";
        action.action = "restore-entry";
        action.payload = entry.label + ":" + std::to_string(entry.state.size());
        if (const auto previous = last_by_session.find(entry.session_id); previous != last_by_session.end()) action.prerequisites.push_back(previous->second);
        if (const auto checkpoint = latest_checkpoint.find(entry.session_id); checkpoint != latest_checkpoint.end()) {
            RecoveryAction checkpoint_action;
            checkpoint_action.sequence = next_sequence++;
            checkpoint_action.session_id = entry.session_id;
            checkpoint_action.generation = checkpoint->second.generation;
            checkpoint_action.component = "checkpoint";
            checkpoint_action.action = "restore-checkpoint";
            checkpoint_action.payload = checkpoint->second.reason;
            if (const auto previous = last_by_session.find(entry.session_id); previous != last_by_session.end()) checkpoint_action.prerequisites.push_back(previous->second);
            actions.push_back(checkpoint_action);
            last_by_session[entry.session_id] = checkpoint_action.sequence;
            action.prerequisites.push_back(checkpoint_action.sequence);
            latest_checkpoint.erase(checkpoint);
        }
        actions.push_back(action);
        last_by_session[entry.session_id] = action.sequence;
    }
    for (const auto& [session_id, checkpoint] : latest_checkpoint) {
        RecoveryAction action;
        action.sequence = next_sequence++;
        action.session_id = session_id;
        action.generation = checkpoint.generation;
        action.component = "checkpoint";
        action.action = "restore-checkpoint";
        action.payload = checkpoint.reason;
        actions.push_back(std::move(action));
    }
    std::sort(actions.begin(), actions.end(), [](const auto& left, const auto& right) { return left.sequence < right.sequence; });
    return actions;
}

std::vector<std::string> RecoveryOrchestrator::validate(const std::vector<RecoveryAction>& actions) const {
    std::vector<std::string> errors;
    std::set<std::uint64_t> sequences;
    std::set<std::uint64_t> available;
    for (const auto& action : actions) {
        if (action.sequence == 0U || !sequences.insert(action.sequence).second) errors.push_back("duplicate or zero recovery sequence");
        if (action.session_id == 0U) errors.push_back("recovery action without session");
        if (action.component.empty() || action.action.empty()) errors.push_back("recovery action missing identity");
        for (const auto prerequisite : action.prerequisites) {
            if (prerequisite >= action.sequence) errors.push_back("forward recovery prerequisite at " + std::to_string(action.sequence));
            if (!available.contains(prerequisite)) errors.push_back("unavailable recovery prerequisite at " + std::to_string(action.sequence));
        }
        available.insert(action.sequence);
    }
    return errors;
}

std::vector<RecoveryAction> RecoveryOrchestrator::ready(const std::vector<RecoveryAction>& actions,
                                                        const std::set<std::uint64_t>& completed) const {
    std::vector<RecoveryAction> result;
    for (const auto& action : actions) {
        if (completed.contains(action.sequence)) continue;
        const auto satisfied = std::all_of(action.prerequisites.begin(), action.prerequisites.end(),
                                           [&](const auto prerequisite) { return completed.contains(prerequisite); });
        if (satisfied) result.push_back(action);
    }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) { return left.sequence < right.sequence; });
    return result;
}

std::string RecoveryOrchestrator::render(const std::vector<RecoveryAction>& actions) const {
    std::vector<std::vector<std::string>> rows{{"sequence", "session", "generation", "component", "action", "prerequisites"}};
    for (const auto& action : actions) {
        std::vector<std::string> prerequisites;
        for (const auto prerequisite : action.prerequisites) prerequisites.push_back(std::to_string(prerequisite));
        rows.push_back({std::to_string(action.sequence), std::to_string(action.session_id), std::to_string(action.generation), action.component, action.action, join_labels(prerequisites, ",")});
    }
    return render_table(rows);
}

}  // namespace aegis::detail
