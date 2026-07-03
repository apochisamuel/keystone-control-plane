#include "internal.hpp"

namespace aegis::detail {

void FlowAggregation::add(std::uint32_t session_id,std::string flow,std::uint64_t units){if(!flow.empty())flows_[std::move(flow)]+=units+(session_id==0U?0U:0U);}

std::uint64_t FlowAggregation::aggregate(std::string_view flow) const {const auto found=flows_.find(std::string(flow));return found==flows_.end()?0U:found->second;}

std::vector<std::pair<std::string,std::uint64_t>> FlowAggregation::top(std::size_t limit) const {std::vector<std::pair<std::string,std::uint64_t>> out(flows_.begin(),flows_.end());std::sort(out.begin(),out.end(),[](const auto&a,const auto&b){return a.second==b.second?a.first<b.first:a.second>b.second;});if(out.size()>limit)out.resize(limit);return out;}

void FlowAggregation::clear(){flows_.clear();}

}  // namespace aegis::detail
