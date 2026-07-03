#include "internal.hpp"

namespace aegis::detail {

std::vector<std::string> RouteAuditor::audit(const Session& session, const CompiledPolicy* policy,
                                             const StableRouteCache& cache) const {
    std::vector<std::string> findings;
    if (session.profile == nullptr) { findings.push_back("profile-missing"); return findings; }
    if (session.profile->routes.empty()) findings.push_back("route-set-empty");
    std::unordered_set<std::uint32_t> ids;
    for (const auto& route : session.profile->routes) {
        if (!ids.insert(route.id).second) findings.push_back("duplicate-route:" + std::to_string(route.id));
        if (!route.enabled) findings.push_back("disabled-route:" + std::to_string(route.id));
        if (route.profile_generation != session.generation) findings.push_back("route-generation-mismatch:" + std::to_string(route.id));
        if (route.next_hop.empty()) findings.push_back("route-hop-empty:" + std::to_string(route.id));
    }
    if (policy != nullptr && policy->route_id != 0U && !ids.contains(policy->route_id)) findings.push_back("policy-route-unbound");
    const auto cached = cache.get(session.id, session.generation);
    if (cached.has_value() && !ids.contains(cached->route_id)) findings.push_back("cached-route-unbound");
    if (findings.empty()) findings.push_back("route-audit-ok");
    return findings;
}

std::string RouteAuditor::render(const std::vector<std::string>& findings) const { return join_labels(findings, ";"); }

}  // namespace aegis::detail
