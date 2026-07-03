#include "internal.hpp"

namespace aegis::detail {

void RetryPlanner::schedule(std::string key,std::uint64_t due){if(key.empty())return;auto& entry=entries_[std::move(key)];entry.key=entry.key.empty()?"":entry.key;entry.due=due;entry.complete=false;}

std::vector<RetryEntry> RetryPlanner::due(std::uint64_t tick,std::uint32_t max_attempts){std::vector<RetryEntry> out;for(auto&[key,entry]:entries_){if(entry.key.empty())entry.key=key;if(!entry.complete&&entry.due<=tick&&entry.attempts<max_attempts){++entry.attempts;entry.due=tick+(1ULL<<std::min(8U,entry.attempts));out.push_back(entry);}}std::sort(out.begin(),out.end(),[](const auto&a,const auto&b){return a.key<b.key;});return out;}

bool RetryPlanner::complete(std::string_view key){auto found=entries_.find(std::string(key));if(found==entries_.end())return false;found->second.complete=true;return true;}

void RetryPlanner::clear(){entries_.clear();}

}  // namespace aegis::detail
