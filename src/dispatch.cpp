#include "internal.hpp"

namespace aegis::detail {

DispatchPlan Dispatcher::build_plan(Session& session, const CompiledPolicy* policy) {
    if (session.profile == nullptr || session.profile->routes.empty()) throw DecodeError("session lacks route");
    DispatchPlan plan;
    plan.session_id = session.id;
    plan.generation = session.generation;
    plan.selected = &session.profile->routes.front();
    plan.key = canonical_text(plan.selected->next_hop);
    for (const auto& route : session.profile->routes) plan.candidate_ids.push_back(route.id);
    if (policy != nullptr) {
        plan.extension_signature = bind_extension_routes(policy->extensions);
        if (!policy->route_label.empty()) plan.key += ":" + policy->route_label;
    }
    return plan;
}

std::string Dispatcher::emit(const DispatchPlan& plan, const Session& session) const {
    if (plan.selected == nullptr) throw DecodeError("empty dispatch plan");
    std::ostringstream output;
    output << "session=" << session.id << ";route=" << plan.selected->id << ";hop="
           << plan.selected->next_hop << ";weight=" << plan.selected->weight
           << ";signature=" << plan.extension_signature;
    return output.str();
}

std::string Dispatcher::dispatch(Session& session, const CompiledPolicy* policy) {
    auto iterator = plans_.find(session.id);
    if (iterator == plans_.end()) {
        iterator = plans_.emplace(session.id, build_plan(session, policy)).first;
    }
    return emit(iterator->second, session);
}

void Dispatcher::invalidate(std::uint32_t session_id) {
    plans_.erase(session_id);
}

void Dispatcher::clear() {
    plans_.clear();
}

std::size_t Dispatcher::size() const {
    return plans_.size();
}

}  // namespace aegis::detail
