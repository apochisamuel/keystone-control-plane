#include "internal.hpp"

namespace aegis::detail {

void QoSProfileStore::put(QoSProfile profile){if(profile.id!=0U)profiles_[profile.id]=std::move(profile);}

std::optional<QoSProfile> QoSProfileStore::get(std::uint32_t id) const {const auto found=profiles_.find(id);return found==profiles_.end()?std::nullopt:std::optional<QoSProfile>(found->second);}

std::vector<QoSProfile> QoSProfileStore::ordered() const {std::vector<QoSProfile> out;for(const auto&[_,profile]:profiles_)out.push_back(profile);std::sort(out.begin(),out.end(),[](const auto&a,const auto&b){return a.priority==b.priority?a.id<b.id:a.priority>b.priority;});return out;}

void QoSProfileStore::clear(){profiles_.clear();}

}  // namespace aegis::detail
