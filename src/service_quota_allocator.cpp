#include "internal.hpp"

namespace aegis::detail {

void ServiceQuotaAllocator::assign(std::string service,std::uint64_t capacity){if(!service.empty())available_[std::move(service)]=capacity;}

std::uint64_t ServiceQuotaAllocator::grant(std::string_view service,std::uint64_t requested){auto found=available_.find(std::string(service));if(found==available_.end())return 0U;const auto granted=std::min(requested,found->second);found->second-=granted;return granted;}

std::string ServiceQuotaAllocator::report() const {std::vector<std::vector<std::string>> rows{{"service","available"}};for(const auto&[service,value]:available_)rows.push_back({service,std::to_string(value)});return render_table(rows);}

void ServiceQuotaAllocator::clear(){available_.clear();}

}  // namespace aegis::detail
