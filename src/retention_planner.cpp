#include "internal.hpp"

namespace aegis::detail {

void RetentionPlanner::remember(std::string key,std::uint64_t tick){if(!key.empty())entries_[std::move(key)]=tick;}

std::vector<std::string> RetentionPlanner::evict(std::uint64_t before_tick){std::vector<std::string> out;for(auto it=entries_.begin();it!=entries_.end();){if(it->second<before_tick){out.push_back(it->first);it=entries_.erase(it);}else ++it;}return out;}

std::vector<std::string> RetentionPlanner::keys() const {std::vector<std::string> out;for(const auto&[key,_]:entries_)out.push_back(key);return out;}

void RetentionPlanner::clear(){entries_.clear();}

}  // namespace aegis::detail
