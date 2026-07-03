#include "internal.hpp"

namespace aegis::detail {

bool ResourceLeaseTable::acquire(std::string key,std::uint32_t holder,std::uint64_t expires){if(key.empty()||holder==0U)return false;const auto found=leases_.find(key);if(found!=leases_.end()&&found->second.holder!=holder&&found->second.expires>=expires)return false;const auto stored=key; leases_[std::move(key)]={stored,holder,expires};return true;}

bool ResourceLeaseTable::renew(std::string_view key,std::uint32_t holder,std::uint64_t expires){auto found=leases_.find(std::string(key));if(found==leases_.end()||found->second.holder!=holder||expires<found->second.expires)return false;found->second.expires=expires;return true;}

bool ResourceLeaseTable::release(std::string_view key,std::uint32_t holder){auto found=leases_.find(std::string(key));if(found==leases_.end()||found->second.holder!=holder)return false;leases_.erase(found);return true;}

std::vector<ResourceLease> ResourceLeaseTable::active(std::uint64_t tick) const {std::vector<ResourceLease> out;for(const auto&[_,lease]:leases_)if(lease.expires>=tick)out.push_back(lease);std::sort(out.begin(),out.end(),[](const auto&a,const auto&b){return a.key<b.key;});return out;}

void ResourceLeaseTable::clear(){leases_.clear();}

}  // namespace aegis::detail
