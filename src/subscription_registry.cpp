#include "internal.hpp"

namespace aegis::detail {

void SubscriptionRegistry::subscribe(std::uint32_t session_id,std::string topic){if(!topic.empty())subscriptions_[session_id].insert(std::move(topic));}

bool SubscriptionRegistry::unsubscribe(std::uint32_t session_id,std::string_view topic){auto found=subscriptions_.find(session_id);if(found==subscriptions_.end())return false;const auto removed=found->second.erase(std::string(topic))!=0U;if(found->second.empty())subscriptions_.erase(found);return removed;}

std::vector<std::string> SubscriptionRegistry::topics(std::uint32_t session_id) const {const auto found=subscriptions_.find(session_id);return found==subscriptions_.end()?std::vector<std::string>{}:std::vector<std::string>(found->second.begin(),found->second.end());}

std::vector<std::uint32_t> SubscriptionRegistry::subscribers(std::string_view topic) const {std::vector<std::uint32_t> out;for(const auto& [session,topics]:subscriptions_)if(topics.contains(std::string(topic)))out.push_back(session);std::sort(out.begin(),out.end());return out;}

void SubscriptionRegistry::clear(){subscriptions_.clear();}

}  // namespace aegis::detail
