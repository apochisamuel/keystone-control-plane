#include "internal.hpp"

namespace aegis::detail {

void NetworkSliceRegistry::attach(std::uint32_t session_id, std::string slice) { if (!slice.empty()) memberships_[session_id]=std::move(slice); }

bool NetworkSliceRegistry::detach(std::uint32_t session_id) { return memberships_.erase(session_id)!=0U; }

std::optional<std::string> NetworkSliceRegistry::slice(std::uint32_t session_id) const { const auto found=memberships_.find(session_id); return found==memberships_.end()?std::nullopt:std::optional<std::string>(found->second); }

std::vector<std::uint32_t> NetworkSliceRegistry::members(std::string_view slice_name) const { std::vector<std::uint32_t> out; for (const auto& [session,slice] : memberships_) if (slice==slice_name) out.push_back(session); std::sort(out.begin(),out.end()); return out; }

void NetworkSliceRegistry::clear() { memberships_.clear(); }

}  // namespace aegis::detail
