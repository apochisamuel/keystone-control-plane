#include "internal.hpp"

namespace aegis::detail {

void TransitWindow::offer(std::uint32_t session_id,std::uint64_t units){if(units!=0U)pending_.push_back({session_id,units});}

std::vector<std::pair<std::uint32_t,std::uint64_t>> TransitWindow::take(std::uint64_t limit){std::vector<std::pair<std::uint32_t,std::uint64_t>> out;std::uint64_t used{};while(!pending_.empty()&&used<limit){auto item=pending_.front();pending_.pop_front();if(item.second>limit-used){pending_.push_front({item.first,item.second-(limit-used)});item.second=limit-used;}out.push_back(item);used+=item.second;}return out;}

std::size_t TransitWindow::size() const{return pending_.size();}

void TransitWindow::clear(){pending_.clear();}

}  // namespace aegis::detail
