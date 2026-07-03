#include "internal.hpp"

namespace aegis::detail {

void PredictiveScheduler::observe(std::string key,std::uint64_t tick){auto& values=observations_[std::move(key)];values.push_back(tick);if(values.size()>16U)values.erase(values.begin());}

std::optional<std::uint64_t> PredictiveScheduler::next(std::string_view key) const {const auto found=observations_.find(std::string(key));if(found==observations_.end()||found->second.size()<2U)return std::nullopt;std::uint64_t total{};for(std::size_t index=1;index<found->second.size();++index)total+=found->second[index]-found->second[index-1U];return found->second.back()+total/(found->second.size()-1U);}

void PredictiveScheduler::clear(){observations_.clear();}

}  // namespace aegis::detail
