#include "internal.hpp"

namespace aegis::detail {

void ChargingTariff::set_rate(std::string category,std::uint64_t micros_per_unit){if(!category.empty())rates_[std::move(category)]=micros_per_unit;}

std::uint64_t ChargingTariff::quote(std::string_view category,std::uint64_t units) const {const auto found=rates_.find(std::string(category));if(found==rates_.end()||found->second==0U||units>std::numeric_limits<std::uint64_t>::max()/found->second)return 0U;return units*found->second;}

std::vector<std::string> ChargingTariff::categories() const {std::vector<std::string> out;for(const auto&[category,_]:rates_)out.push_back(category);std::sort(out.begin(),out.end());return out;}

void ChargingTariff::clear(){rates_.clear();}

}  // namespace aegis::detail
