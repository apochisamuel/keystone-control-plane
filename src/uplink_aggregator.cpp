#include "internal.hpp"

namespace aegis::detail {

void UplinkAggregator::add(std::uint32_t session_id,std::uint64_t units){totals_[session_id]+=units;}

std::uint64_t UplinkAggregator::total(std::uint32_t session_id) const {const auto found=totals_.find(session_id);return found==totals_.end()?0U:found->second;}

std::vector<std::pair<std::uint32_t,std::uint64_t>> UplinkAggregator::top(std::size_t limit) const {std::vector<std::pair<std::uint32_t,std::uint64_t>> out(totals_.begin(),totals_.end());std::sort(out.begin(),out.end(),[](const auto&a,const auto&b){return a.second==b.second?a.first<b.first:a.second>b.second;});if(out.size()>limit)out.resize(limit);return out;}

void UplinkAggregator::clear(){totals_.clear();}

}  // namespace aegis::detail
