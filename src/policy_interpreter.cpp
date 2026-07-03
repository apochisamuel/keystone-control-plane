#include "internal.hpp"

namespace aegis::detail {

void PolicyInterpreter::load(std::uint32_t session_id, std::vector<RouteRule> rules) { rules_[session_id]=std::move(rules); }

std::optional<std::uint32_t> PolicyInterpreter::evaluate(std::uint32_t session_id, std::string_view payload) const {
    const auto found=rules_.find(session_id); if(found==rules_.end()) return std::nullopt;
    for (const auto& rule : found->second) { if (rule.match.empty() || payload.find(rule.match)!=std::string_view::npos) return rule.route_id; }
    return std::nullopt;
}

std::size_t PolicyInterpreter::rule_count(std::uint32_t session_id) const { const auto found=rules_.find(session_id); return found==rules_.end()?0U:found->second.size(); }

void PolicyInterpreter::clear() { rules_.clear(); }

}  // namespace aegis::detail
