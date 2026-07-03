#include "internal.hpp"

namespace aegis::detail {

std::uint64_t OperationQueue::push(std::uint64_t due, std::uint32_t session_id, std::string kind) { const auto id=next_id_++; items_.push_back({id,due,session_id,std::move(kind),false}); return id; }

std::vector<QueuedOperation> OperationQueue::pop(std::uint64_t tick, std::size_t limit) {
    std::vector<QueuedOperation> ready; std::sort(items_.begin(),items_.end(),[](const auto& a,const auto& b){return a.due==b.due?a.id<b.id:a.due<b.due;});
    auto it=items_.begin(); while(it!=items_.end()) { if (it->cancelled || (it->due<=tick && ready.size()<limit)) { if (!it->cancelled) ready.push_back(*it); it=items_.erase(it); } else ++it; } return ready;
}

bool OperationQueue::cancel(std::uint64_t id) { const auto found=std::find_if(items_.begin(),items_.end(),[id](const auto& item){return item.id==id;}); if(found==items_.end()) return false; found->cancelled=true; return true; }

std::size_t OperationQueue::size() const { return static_cast<std::size_t>(std::count_if(items_.begin(),items_.end(),[](const auto& item){return !item.cancelled;})); }

void OperationQueue::clear() { items_.clear(); next_id_=1U; }

}  // namespace aegis::detail
