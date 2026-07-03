#include "internal.hpp"

namespace aegis::detail {

void AccessPatternProfiler::observe(std::uint32_t session_id,Operation operation){++patterns_[session_id][describe_operation(operation)];}

std::vector<std::pair<std::string,std::uint64_t>> AccessPatternProfiler::summary(std::uint32_t session_id) const {const auto found=patterns_.find(session_id);if(found==patterns_.end())return {};return {found->second.begin(),found->second.end()};}

void AccessPatternProfiler::clear(){patterns_.clear();}

}  // namespace aegis::detail
