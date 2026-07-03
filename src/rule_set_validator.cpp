#include "internal.hpp"

namespace aegis::detail {

std::vector<std::string> RuleSetValidator::validate(const std::vector<RouteRule>& rules) const {std::vector<std::string> errors;std::set<std::uint32_t> routes;for(const auto& rule:rules){if(rule.route_id==0U)errors.push_back("zero route id");if(rule.quota==0U)errors.push_back("zero quota for route "+std::to_string(rule.route_id));if(!routes.insert(rule.route_id).second)errors.push_back("duplicate route "+std::to_string(rule.route_id));}return errors;}

std::vector<RouteRule> RuleSetValidator::normalize(std::vector<RouteRule> rules) const {std::sort(rules.begin(),rules.end(),[](const auto&a,const auto&b){return a.route_id==b.route_id?a.version<b.version:a.route_id<b.route_id;});rules.erase(std::unique(rules.begin(),rules.end(),[](const auto&a,const auto&b){return a.route_id==b.route_id;}),rules.end());return rules;}

}  // namespace aegis::detail
