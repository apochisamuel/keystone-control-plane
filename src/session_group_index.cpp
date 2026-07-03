#include "internal.hpp"

namespace aegis::detail {

void SessionGroupIndex::put(std::uint32_t session_id,std::string group){if(!group.empty())groups_[session_id]=std::move(group);}

bool SessionGroupIndex::erase(std::uint32_t session_id){return groups_.erase(session_id)!=0U;}

std::vector<std::uint32_t> SessionGroupIndex::find(std::string_view group) const {std::vector<std::uint32_t> out;for(const auto&[session,name]:groups_)if(name==group)out.push_back(session);std::sort(out.begin(),out.end());return out;}

void SessionGroupIndex::clear(){groups_.clear();}

}  // namespace aegis::detail
