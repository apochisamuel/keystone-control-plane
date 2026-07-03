#include "internal.hpp"

namespace aegis::detail {

std::uint64_t PolicySchedule::schedule(std::uint32_t session_id,std::uint64_t due,std::string label){const auto id=next_id_++;entries_.push_back({id,session_id,due,std::move(label)});return id;}

std::vector<ScheduledPolicy> PolicySchedule::due(std::uint64_t tick){std::vector<ScheduledPolicy> out;auto it=entries_.begin();while(it!=entries_.end()){if(it->due<=tick){out.push_back(*it);it=entries_.erase(it);}else ++it;}std::sort(out.begin(),out.end(),[](const auto&a,const auto&b){return a.due==b.due?a.id<b.id:a.due<b.due;});return out;}

void PolicySchedule::clear(){entries_.clear();next_id_=1U;}

}  // namespace aegis::detail
