#include "internal.hpp"

namespace aegis::detail {

void SessionArchive::append(std::uint32_t session_id,std::uint64_t generation,std::uint64_t tick,std::string state){records_[session_id].push_back({session_id,generation,tick,std::move(state)});}

std::optional<ArchivedSession> SessionArchive::latest(std::uint32_t session_id) const {const auto found=records_.find(session_id);if(found==records_.end()||found->second.empty())return std::nullopt;return found->second.back();}

std::size_t SessionArchive::prune(std::uint32_t session_id,std::size_t keep){auto found=records_.find(session_id);if(found==records_.end()||found->second.size()<=keep)return 0U;const auto removed=found->second.size()-keep;found->second.erase(found->second.begin(),found->second.begin()+static_cast<std::ptrdiff_t>(removed));return removed;}

void SessionArchive::clear(){records_.clear();}

}  // namespace aegis::detail
