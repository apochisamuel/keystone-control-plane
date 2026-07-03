#include "internal.hpp"

namespace aegis::detail {

void ReplicaSetPlanner::plan(std::uint32_t session_id,std::vector<std::string> replicas){std::deque<std::string> clean;for(auto& replica:replicas)if(!replica.empty()&&std::find(clean.begin(),clean.end(),replica)==clean.end())clean.push_back(std::move(replica));replicas_[session_id]=std::move(clean);}

std::optional<std::string> ReplicaSetPlanner::promote(std::uint32_t session_id){auto found=replicas_.find(session_id);if(found==replicas_.end()||found->second.empty())return std::nullopt;auto value=found->second.front();found->second.pop_front();found->second.push_back(value);return value;}

std::vector<std::string> ReplicaSetPlanner::members(std::uint32_t session_id) const {const auto found=replicas_.find(session_id);return found==replicas_.end()?std::vector<std::string>{}:std::vector<std::string>(found->second.begin(),found->second.end());}

void ReplicaSetPlanner::clear(){replicas_.clear();}

}  // namespace aegis::detail
