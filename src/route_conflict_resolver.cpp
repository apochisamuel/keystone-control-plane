#include "internal.hpp"

namespace aegis::detail {

std::pair<std::vector<Route>,std::vector<RouteConflict>> RouteConflictResolver::resolve(std::vector<Route> routes) const {std::unordered_map<std::uint32_t,Route> selected;std::unordered_map<std::uint32_t,RouteConflict> conflicts;for(auto& route:routes){auto found=selected.find(route.id);if(found==selected.end()){selected.emplace(route.id,std::move(route));continue;}auto& conflict=conflicts[route.id];conflict.route_id=route.id;const auto preserve=route.profile_generation>found->second.profile_generation||(route.profile_generation==found->second.profile_generation&&route.weight>found->second.weight);if(preserve){conflict.discarded.push_back(found->second.next_hop);found->second=std::move(route);}else conflict.discarded.push_back(route.next_hop);}std::vector<Route> output;for(auto&[_,route]:selected)output.push_back(std::move(route));std::sort(output.begin(),output.end(),[](const auto&a,const auto&b){return a.id<b.id;});std::vector<RouteConflict> issues;for(auto&[_,item]:conflicts)issues.push_back(std::move(item));std::sort(issues.begin(),issues.end(),[](const auto&a,const auto&b){return a.route_id<b.route_id;});return {std::move(output),std::move(issues)};}

std::string RouteConflictResolver::render(const std::vector<RouteConflict>& conflicts) const {std::vector<std::vector<std::string>> rows{{"route","discarded"}};for(const auto& conflict:conflicts)rows.push_back({std::to_string(conflict.route_id),join_labels(conflict.discarded,",")});return render_table(rows);}

}  // namespace aegis::detail
