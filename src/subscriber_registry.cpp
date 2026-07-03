#include "internal.hpp"

namespace aegis::detail {

void SubscriberRegistry::upsert(std::uint32_t session_id,std::string subscriber_name,std::string segment_name){if(session_id!=0U&&!subscriber_name.empty())entries_[session_id]={std::move(subscriber_name),std::move(segment_name)};}

std::optional<std::string> SubscriberRegistry::subscriber(std::uint32_t session_id) const {const auto found=entries_.find(session_id);return found==entries_.end()?std::nullopt:std::optional<std::string>(found->second.subscriber);}

std::vector<std::uint32_t> SubscriberRegistry::segment(std::string_view name) const {std::vector<std::uint32_t> out;for(const auto&[session,entry]:entries_)if(entry.segment==name)out.push_back(session);std::sort(out.begin(),out.end());return out;}

void SubscriberRegistry::clear(){entries_.clear();}

}  // namespace aegis::detail
