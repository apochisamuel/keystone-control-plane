#include "internal.hpp"

namespace aegis::detail {

std::vector<IntegrityCheck> IntegrityVerifier::session(const Session& session) const {
    std::vector<IntegrityCheck> checks;
    checks.push_back({"session.id", session.id != 0U, session.id == 0U ? "zero session id" : "set"});
    checks.push_back({"session.profile", session.profile != nullptr, session.profile == nullptr ? "missing profile" : "present"});
    checks.push_back({"session.generation", session.profile == nullptr || session.profile->generation == session.generation,
                      session.profile == nullptr ? "not applicable" : "profile/session generation"});
    if (session.profile != nullptr) checks.push_back({"session.route", !session.profile->routes.empty(), "routes=" + std::to_string(session.profile->routes.size())});
    checks.push_back({"session.quota", session.quota > 0U, "quota=" + std::to_string(session.quota)});
    return checks;
}

std::vector<IntegrityCheck> IntegrityVerifier::policy(const CompiledPolicy* policy) const {
    if (policy == nullptr) return {{"policy.present", true, "no active policy"}};
    std::vector<IntegrityCheck> checks;
    checks.push_back({"policy.session", policy->session_id != 0U, "session=" + std::to_string(policy->session_id)});
    checks.push_back({"policy.route", policy->route_id != 0U, "route=" + std::to_string(policy->route_id)});
    checks.push_back({"policy.attachment", policy->attachment != nullptr, policy->attachment == nullptr ? "missing" : "present"});
    checks.push_back({"policy.rules", !policy->rules.empty(), "rules=" + std::to_string(policy->rules.size())});
    return checks;
}

std::vector<IntegrityCheck> IntegrityVerifier::topology(const TopologyGraph& graph) const {
    std::vector<IntegrityCheck> checks;
    checks.push_back({"topology.nodes", !graph.nodes.empty(), "nodes=" + std::to_string(graph.nodes.size())});
    bool roots_in_range = true;
    for (const auto root : graph.roots) if (root >= graph.nodes.size()) roots_in_range = false;
    checks.push_back({"topology.roots", roots_in_range, "roots=" + std::to_string(graph.roots.size())});
    return checks;
}

std::string IntegrityVerifier::summarize(const std::vector<IntegrityCheck>& checks) const {
    std::ostringstream output;
    for (const auto& check : checks) output << check.name << "=" << (check.passed ? "pass" : "fail") << ":" << check.detail << ";";
    return output.str();
}

}  // namespace aegis::detail
