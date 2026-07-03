#include "internal.hpp"

namespace aegis::detail {

void ServiceHealthMatrix::observe(std::string service,std::string zone,bool healthy_value){if(!service.empty()&&!zone.empty())matrix_[{std::move(service),std::move(zone)}]=healthy_value;}

bool ServiceHealthMatrix::healthy(std::string_view service,std::string_view zone) const {const auto found=matrix_.find({std::string(service),std::string(zone)});return found!=matrix_.end()&&found->second;}

std::vector<std::string> ServiceHealthMatrix::unhealthy() const {std::vector<std::string> out;for(const auto&[key,value]:matrix_)if(!value)out.push_back(key.first+"@"+key.second);return out;}

void ServiceHealthMatrix::clear(){matrix_.clear();}

}  // namespace aegis::detail
