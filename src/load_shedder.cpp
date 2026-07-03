#include "internal.hpp"

namespace aegis::detail {

void LoadShedder::configure(std::uint64_t threshold,std::uint32_t minimum_priority){threshold_=threshold;minimum_priority_=minimum_priority;}

LoadDecision LoadShedder::decide(std::uint64_t observed,std::uint32_t priority) const {if(observed<=threshold_)return {true,priority,"within threshold"};if(priority>=minimum_priority_)return {true,priority,"priority bypass"};return {false,priority,"shed"};}

void LoadShedder::clear(){threshold_=1000U;minimum_priority_=1U;}

}  // namespace aegis::detail
