#include "internal.hpp"

namespace aegis::detail {

std::uint64_t TimerWheel::schedule(std::uint64_t due,std::string key){const auto id=next_id_++;events_.push_back({id,due,std::move(key),false});return id;}

std::vector<TimerEvent> TimerWheel::advance(std::uint64_t tick){std::vector<TimerEvent> out;auto it=events_.begin();while(it!=events_.end()){if(it->cancelled||it->due<=tick){if(!it->cancelled)out.push_back(*it);it=events_.erase(it);}else ++it;}std::sort(out.begin(),out.end(),[](const auto&a,const auto&b){return a.due==b.due?a.id<b.id:a.due<b.due;});return out;}

bool TimerWheel::cancel(std::uint64_t id){const auto found=std::find_if(events_.begin(),events_.end(),[id](const auto&item){return item.id==id;});if(found==events_.end())return false;found->cancelled=true;return true;}

void TimerWheel::clear(){events_.clear();next_id_=1U;}

}  // namespace aegis::detail
