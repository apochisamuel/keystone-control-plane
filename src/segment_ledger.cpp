#include "internal.hpp"

namespace aegis::detail {

void SegmentLedger::append(std::uint32_t session_id,std::uint32_t segment,std::uint64_t units){entries_[session_id].push_back({segment,units});}

std::string SegmentLedger::summary(std::uint32_t session_id) const {const auto found=entries_.find(session_id);if(found==entries_.end())return "";std::uint64_t total{};std::vector<std::string> labels;for(const auto&[segment,units]:found->second){total+=units;labels.push_back(std::to_string(segment)+"="+std::to_string(units));}return "total="+std::to_string(total)+";segments="+join_labels(labels,",");}

std::size_t SegmentLedger::trim(std::uint32_t session_id,std::size_t limit){auto found=entries_.find(session_id);if(found==entries_.end()||found->second.size()<=limit)return 0U;const auto removed=found->second.size()-limit;found->second.erase(found->second.begin(),found->second.begin()+static_cast<std::ptrdiff_t>(removed));return removed;}

void SegmentLedger::clear(){entries_.clear();}

}  // namespace aegis::detail
