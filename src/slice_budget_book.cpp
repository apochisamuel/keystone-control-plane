#include "internal.hpp"

namespace aegis::detail {

void SliceBudgetBook::configure(std::string slice,std::uint64_t budget){if(!slice.empty())budgets_[std::move(slice)]=budget;}

bool SliceBudgetBook::consume(std::string_view slice,std::uint64_t units){auto found=budgets_.find(std::string(slice));if(found==budgets_.end()||units>found->second)return false;found->second-=units;return true;}

std::uint64_t SliceBudgetBook::remaining(std::string_view slice) const {const auto found=budgets_.find(std::string(slice));return found==budgets_.end()?0U:found->second;}

void SliceBudgetBook::clear(){budgets_.clear();}

}  // namespace aegis::detail
