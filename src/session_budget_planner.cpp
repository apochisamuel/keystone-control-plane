#include "internal.hpp"

namespace aegis::detail {

void SessionBudgetPlanner::configure(std::uint32_t session_id,std::uint64_t budget){remaining_[session_id]=budget;}

bool SessionBudgetPlanner::consume(std::uint32_t session_id,std::uint64_t units){auto found=remaining_.find(session_id);if(found==remaining_.end()||units>found->second)return false;found->second-=units;return true;}

std::uint64_t SessionBudgetPlanner::remaining(std::uint32_t session_id) const {const auto found=remaining_.find(session_id);return found==remaining_.end()?0U:found->second;}

void SessionBudgetPlanner::clear(){remaining_.clear();}

}  // namespace aegis::detail
