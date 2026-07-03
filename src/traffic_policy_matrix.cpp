#include "internal.hpp"

namespace aegis::detail {

void TrafficPolicyMatrix::set(std::string source,std::string destination,bool allowed_value){if(!source.empty()&&!destination.empty())rules_[{std::move(source),std::move(destination)}]=allowed_value;}

bool TrafficPolicyMatrix::allowed(std::string_view source,std::string_view destination) const {const auto found=rules_.find({std::string(source),std::string(destination)});return found==rules_.end()?true:found->second;}

void TrafficPolicyMatrix::clear(){rules_.clear();}

}  // namespace aegis::detail
