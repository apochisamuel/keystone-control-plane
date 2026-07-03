#include "internal.hpp"

namespace aegis::detail {

std::vector<std::string> validate_policy_candidate(const PolicyCandidate& candidate) {
    std::vector<std::string> issues;
    if (candidate.session_id == 0U) issues.push_back("policy session is zero");
    if (candidate.profile_id == 0U) issues.push_back("policy profile is zero");
    if (candidate.route_id == 0U) issues.push_back("policy route is zero");
    if (candidate.version == 0U) issues.push_back("policy version is zero");
    if (candidate.next_hop.empty()) issues.push_back("policy next hop is empty");
    if (candidate.extension.size() > 65535U) issues.push_back("policy extension exceeds limit");
    if (!candidate.extension.empty()) {
        try {
            auto decoded = decode_extensions(candidate.extension);
            if (decoded.size() > 128U) issues.push_back("policy contains too many extensions");
        } catch (const DecodeError& error) {
            issues.push_back(std::string("extension rejected: ") + error.what());
        }
    }
    return issues;
}

}  // namespace aegis::detail
