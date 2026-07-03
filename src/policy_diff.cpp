#include "internal.hpp"

namespace aegis::detail {

namespace {
void add_difference(std::vector<PolicyDifference>& output, std::string field, std::string before,
                    std::string after, std::string impact) {
    if (before != after) output.push_back(PolicyDifference{std::move(field), std::move(before), std::move(after), std::move(impact)});
}
}

std::vector<PolicyDifference> PolicyDiffer::compare(const PolicyCandidate& before, const PolicyCandidate& after) const {
    std::vector<PolicyDifference> differences;
    add_difference(differences, "session", std::to_string(before.session_id), std::to_string(after.session_id), "identity scope");
    add_difference(differences, "profile", std::to_string(before.profile_id), std::to_string(after.profile_id), "profile replacement");
    add_difference(differences, "route", std::to_string(before.route_id), std::to_string(after.route_id), "dispatch selection");
    add_difference(differences, "version", std::to_string(before.version), std::to_string(after.version), "configuration generation");
    add_difference(differences, "next-hop", canonical_text(before.next_hop), canonical_text(after.next_hop), "transport endpoint");
    add_difference(differences, "label", canonical_text(before.label), canonical_text(after.label), "routing preference");
    add_difference(differences, "extension", hex(before.extension), hex(after.extension), "extension evaluation");
    add_difference(differences, "mirror-audit", before.mirror_audit ? "true" : "false", after.mirror_audit ? "true" : "false", "audit replication");
    add_difference(differences, "route-mode", before.route_mode ? "true" : "false", after.route_mode ? "true" : "false", "extension binding");
    return differences;
}

std::string PolicyDiffer::summarize(const std::vector<PolicyDifference>& differences) const {
    std::ostringstream output;
    output << "policy-diff.count=" << differences.size() << ";";
    for (const auto& difference : differences) {
        output << difference.field << "=" << difference.before << "->" << difference.after
               << "(" << difference.impact << ");";
    }
    return output.str();
}

}  // namespace aegis::detail
