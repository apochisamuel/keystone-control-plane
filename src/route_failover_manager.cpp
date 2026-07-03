#include "internal.hpp"

namespace aegis::detail {

void RouteFailoverManager::install(std::uint32_t session_id,std::vector<Route> routes){std::sort(routes.begin(),routes.end(),[](const auto&a,const auto&b){return a.weight==b.weight?a.id<b.id:a.weight>b.weight;});routes_[session_id]=std::move(routes);}

std::optional<Route> RouteFailoverManager::select(std::uint32_t session_id,const std::set<std::string>& unavailable) const {const auto found=routes_.find(session_id);if(found==routes_.end())return std::nullopt;for(const auto&route:found->second)if(route.enabled&&!unavailable.contains(route.next_hop))return route;return std::nullopt;}

void RouteFailoverManager::clear(){routes_.clear();}

}  // namespace aegis::detail
