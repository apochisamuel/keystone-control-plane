#include "internal.hpp"

namespace aegis::detail {

void IdentityCorrelation::observe(std::uint32_t session_id,std::string subscriber,std::string endpoint){if(!subscriber.empty())items_[session_id]={std::move(subscriber),std::move(endpoint)};}

std::vector<std::uint32_t> IdentityCorrelation::related(std::string_view subscriber) const {std::vector<std::uint32_t> out;for(const auto&[session,item]:items_)if(item.subscriber==subscriber)out.push_back(session);std::sort(out.begin(),out.end());return out;}

std::size_t IdentityCorrelation::remove(std::uint32_t session_id){return items_.erase(session_id);}

void IdentityCorrelation::clear(){items_.clear();}

}  // namespace aegis::detail
