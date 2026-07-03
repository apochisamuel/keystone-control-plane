#include "internal.hpp"

namespace aegis::detail {

void ZoneRegistry::assign(std::uint32_t session_id,std::string zone){if(!zone.empty())zones_[session_id]=std::move(zone);}

std::optional<std::string> ZoneRegistry::zone(std::uint32_t session_id) const {const auto found=zones_.find(session_id);return found==zones_.end()?std::nullopt:std::optional<std::string>(found->second);}

std::vector<std::uint32_t> ZoneRegistry::occupants(std::string_view zone_name) const {std::vector<std::uint32_t> out;for(const auto&[session,zone]:zones_)if(zone==zone_name)out.push_back(session);std::sort(out.begin(),out.end());return out;}

void ZoneRegistry::clear(){zones_.clear();}

}  // namespace aegis::detail
