#include "internal.hpp"

namespace aegis::detail {

void WindowedRateEstimator::observe(std::string key,std::uint64_t tick,std::uint64_t units){auto& values=samples_[std::move(key)];values.push_back({tick,units});while(values.size()>128U)values.pop_front();}

std::uint64_t WindowedRateEstimator::rate(std::string_view key,std::uint64_t from_tick) const {const auto found=samples_.find(std::string(key));if(found==samples_.end())return 0U;std::uint64_t total{};std::uint64_t first{};std::uint64_t last{};for(const auto&[tick,units]:found->second)if(tick>=from_tick){if(first==0U)first=tick;last=tick;total+=units;}if(first==0U)return 0U;const auto span=std::max<std::uint64_t>(1U,last-first+1U);return total/span;}

void WindowedRateEstimator::clear(){samples_.clear();}

}  // namespace aegis::detail
