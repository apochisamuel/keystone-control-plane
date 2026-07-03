#include "internal.hpp"

namespace aegis::detail {

void ErrorBudget::configure(std::string scope,std::uint64_t allowance){if(!scope.empty())remaining_[std::move(scope)]=allowance;}

bool ErrorBudget::record(std::string_view scope,std::uint64_t amount){auto found=remaining_.find(std::string(scope));if(found==remaining_.end()||amount>found->second)return false;found->second-=amount;return true;}

std::uint64_t ErrorBudget::remaining(std::string_view scope) const {const auto found=remaining_.find(std::string(scope));return found==remaining_.end()?0U:found->second;}

void ErrorBudget::clear(){remaining_.clear();}

}  // namespace aegis::detail
