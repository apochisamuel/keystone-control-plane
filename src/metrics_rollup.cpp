#include "internal.hpp"

namespace aegis::detail {

void MetricsRollup::observe(std::string metric,std::uint64_t value){auto& aggregate=totals_[std::move(metric)];aggregate.total+=value;++aggregate.count;}

std::uint64_t MetricsRollup::total(std::string_view metric) const {const auto found=totals_.find(std::string(metric));return found==totals_.end()?0U:found->second.total;}

std::optional<std::uint64_t> MetricsRollup::average(std::string_view metric) const {const auto found=totals_.find(std::string(metric));if(found==totals_.end()||found->second.count==0U)return std::nullopt;return found->second.total/found->second.count;}

void MetricsRollup::clear(){totals_.clear();}

}  // namespace aegis::detail
