#include "internal.hpp"

namespace aegis::detail {

void AuditRetentionIndex::retain(AuditRetentionItem item){items_.push_back(std::move(item));}

std::vector<AuditRetentionItem> AuditRetentionIndex::query(std::uint32_t session_id) const {std::vector<AuditRetentionItem> out;for(const auto& item:items_)if(item.session_id==session_id)out.push_back(item);std::sort(out.begin(),out.end(),[](const auto&a,const auto&b){return a.sequence<b.sequence;});return out;}

std::size_t AuditRetentionIndex::trim(std::uint64_t before_tick){const auto before=items_.size();items_.erase(std::remove_if(items_.begin(),items_.end(),[before_tick](const auto& item){return item.tick<before_tick;}),items_.end());return before-items_.size();}

void AuditRetentionIndex::clear(){items_.clear();}

}  // namespace aegis::detail
