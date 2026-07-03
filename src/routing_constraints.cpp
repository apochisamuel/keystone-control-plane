#include "internal.hpp"

namespace aegis::detail {

void RoutingConstraints::set(std::uint32_t session_id,std::vector<std::string> blocked){auto& values=blocked_[session_id];values.clear();for(auto& route:blocked)if(!route.empty())values.insert(std::move(route));}

bool RoutingConstraints::allows(std::uint32_t session_id,std::string_view route) const {const auto found=blocked_.find(session_id);return found==blocked_.end()||!found->second.contains(std::string(route));}

std::vector<std::string> RoutingConstraints::blocked(std::uint32_t session_id) const {const auto found=blocked_.find(session_id);return found==blocked_.end()?std::vector<std::string>{}:std::vector<std::string>(found->second.begin(),found->second.end());}

void RoutingConstraints::clear(){blocked_.clear();}

}  // namespace aegis::detail
