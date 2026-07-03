#include "internal.hpp"

namespace aegis::detail {

std::vector<RouteDecision> RoutePlanner::rank(const Session& session, const CompiledPolicy* policy) const {
    std::vector<RouteDecision> result;
    if (session.profile == nullptr) return result;
    for (const auto& route : session.profile->routes) {
        if (!route.enabled) continue;
        RouteDecision decision;
        decision.route_id = route.id;
        decision.next_hop = route.next_hop;
        decision.score = static_cast<std::uint64_t>(route.weight) * 100U;
        decision.reasons.push_back("weight=" + std::to_string(route.weight));
        if (route.profile_generation == session.generation) { decision.score += 50U; decision.reasons.push_back("generation-match"); }
        if (policy != nullptr && policy->route_id == route.id) { decision.score += 80U; decision.reasons.push_back("policy-route"); }
        if (policy != nullptr && !policy->route_label.empty() && std::find(route.labels.begin(), route.labels.end(), policy->route_label) != route.labels.end()) {
            decision.score += 30U; decision.reasons.push_back("label-match");
        }
        result.push_back(std::move(decision));
    }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
        if (left.score != right.score) return left.score > right.score;
        return left.route_id < right.route_id;
    });
    return result;
}

std::optional<RouteDecision> RoutePlanner::select(const Session& session, const CompiledPolicy* policy,
                                                  std::string_view requested_label) const {
    auto candidates = rank(session, policy);
    const auto ranked = candidates;
    const auto label = canonical_text(requested_label);
    if (!label.empty()) {
        candidates.erase(std::remove_if(candidates.begin(), candidates.end(), [&](const RouteDecision& decision) {
            return decision.next_hop.find(label) == std::string::npos;
        }), candidates.end());
    }
    // A policy label is a preference. A route remains usable when no current
    // next-hop advertises it, so callers retain the highest ranked fallback.
    if (candidates.empty()) candidates = ranked;
    return candidates.empty() ? std::nullopt : std::optional<RouteDecision>(candidates.front());
}

std::string RoutePlanner::explain(const RouteDecision& decision) const {
    std::ostringstream output;
    output << "route=" << decision.route_id << ";hop=" << decision.next_hop << ";score=" << decision.score << ";";
    for (const auto& reason : decision.reasons) output << reason << ";";
    return output.str();
}

}  // namespace aegis::detail
