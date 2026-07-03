#include "internal.hpp"

namespace aegis::detail {

void LatencyHistogram::observe(std::string key,std::uint64_t value){auto& values=values_[std::move(key)];values.push_back(value);if(values.size()>256U)values.erase(values.begin(),values.begin()+static_cast<std::ptrdiff_t>(values.size()-256U));}

std::optional<std::uint64_t> LatencyHistogram::percentile(std::string_view key,std::uint32_t percent) const {const auto found=values_.find(std::string(key));if(found==values_.end()||found->second.empty())return std::nullopt;auto values=found->second;std::sort(values.begin(),values.end());const auto bounded=std::min(100U,percent);const auto index=std::min(values.size()-1U,(values.size()-1U)*bounded/100U);return values[index];}

std::map<std::uint64_t,std::uint64_t> LatencyHistogram::buckets(std::string_view key) const {std::map<std::uint64_t,std::uint64_t> out;const auto found=values_.find(std::string(key));if(found==values_.end())return out;for(auto value:found->second){std::uint64_t bucket=1U;while(bucket<value&&bucket<(1ULL<<62U))bucket<<=1U;++out[bucket];}return out;}

void LatencyHistogram::clear(){values_.clear();}

}  // namespace aegis::detail
