#include "internal.hpp"

namespace aegis::detail {

SessionPhase SessionStateMachine::infer(const Session& session, const CompiledPolicy* policy) const {
    if (session.closed) return SessionPhase::Closed;
    if (session.staged_profile != nullptr) return SessionPhase::Staged;
    if (!session.tunnels.bound().empty()) return SessionPhase::Moving;
    if (policy != nullptr) return SessionPhase::Active;
    if (!session.subscriber.normalized.empty() && session.accepted_frames > 1U) return SessionPhase::Registered;
    return SessionPhase::New;
}

StateTransition SessionStateMachine::transition(SessionPhase current, Operation operation) const {
    StateTransition result;
    result.from = current;
    result.operation = operation;
    result.to = current;
    switch (operation) {
        case Operation::Open:
            result.allowed = current == SessionPhase::New || current == SessionPhase::Closed;
            result.to = SessionPhase::New;
            result.reason = result.allowed ? "new session" : "already opened";
            break;
        case Operation::Register:
            result.allowed = current == SessionPhase::New || current == SessionPhase::Registered;
            result.to = SessionPhase::Registered;
            result.reason = result.allowed ? "subscriber registration" : "registration requires opened session";
            break;
        case Operation::StagePolicy:
        case Operation::Extensions:
            result.allowed = current != SessionPhase::Closed;
            result.to = SessionPhase::Staged;
            result.reason = result.allowed ? "policy staged" : "closed session";
            break;
        case Operation::Commit:
            result.allowed = current == SessionPhase::Staged;
            result.to = SessionPhase::Active;
            result.reason = result.allowed ? "policy active" : "commit requires stage";
            break;
        case Operation::Data:
            result.allowed = current == SessionPhase::Active || current == SessionPhase::Registered || current == SessionPhase::New;
            result.to = current == SessionPhase::New ? SessionPhase::New : current;
            result.reason = result.allowed ? "data accepted" : "data requires live session";
            break;
        case Operation::Handover:
            result.allowed = current == SessionPhase::Active || current == SessionPhase::Registered;
            result.to = SessionPhase::Moving;
            result.reason = result.allowed ? "handover begins" : "handover requires active route";
            break;
        case Operation::Rollback:
            result.allowed = current == SessionPhase::Staged || current == SessionPhase::Active;
            result.to = SessionPhase::Registered;
            result.reason = result.allowed ? "rollback restores prior state" : "nothing to roll back";
            break;
        case Operation::Close:
            result.allowed = current != SessionPhase::Closed;
            result.to = SessionPhase::Closed;
            result.reason = result.allowed ? "session closed" : "already closed";
            break;
        default:
            result.allowed = current != SessionPhase::Closed;
            result.reason = result.allowed ? "observability operation" : "closed session";
            break;
    }
    return result;
}

std::vector<StateTransition> SessionStateMachine::validate(const std::vector<Operation>& operations) const {
    std::vector<StateTransition> result;
    auto current = SessionPhase::New;
    for (const auto operation : operations) {
        auto transition_result = transition(current, operation);
        if (transition_result.allowed) current = transition_result.to;
        result.push_back(std::move(transition_result));
    }
    return result;
}

std::string SessionStateMachine::name(SessionPhase phase) const {
    switch (phase) {
        case SessionPhase::New: return "new";
        case SessionPhase::Registered: return "registered";
        case SessionPhase::Staged: return "staged";
        case SessionPhase::Active: return "active";
        case SessionPhase::Moving: return "moving";
        case SessionPhase::Closed: return "closed";
    }
    return "unknown";
}

}  // namespace aegis::detail
