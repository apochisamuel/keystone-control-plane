#include "internal.hpp"

namespace aegis::detail {

void TelemetrySampler::sample(std::string metric,std::uint64_t value,std::size_t limit){auto& values=samples_[std::move(metric)];values.push_back(value);while(values.size()>limit)values.pop_front();}

std::optional<std::uint64_t> TelemetrySampler::average(std::string_view metric) const {const auto found=samples_.find(std::string(metric));if(found==samples_.end()||found->second.empty())return std::nullopt;std::uint64_t total{};for(auto value:found->second)total+=value;return total/found->second.size();}

std::vector<std::uint64_t> TelemetrySampler::series(std::string_view metric) const {const auto found=samples_.find(std::string(metric));return found==samples_.end()?std::vector<std::uint64_t>{}:std::vector<std::uint64_t>(found->second.begin(),found->second.end());}

void TelemetrySampler::clear(){samples_.clear();}

}  // namespace aegis::detail
