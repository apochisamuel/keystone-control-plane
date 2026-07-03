#include "internal.hpp"

namespace aegis::detail {

void CaptureReplayBuffer::append(PacketRecord record,std::size_t limit){records_.push_back(std::move(record));while(records_.size()>limit)records_.pop_front();}

std::vector<PacketRecord> CaptureReplayBuffer::since(std::uint64_t ordinal) const {std::vector<PacketRecord> out;for(const auto& record:records_)if(record.ordinal>=ordinal)out.push_back(record);return out;}

std::size_t CaptureReplayBuffer::trim(std::size_t limit){if(records_.size()<=limit)return 0U;const auto removed=records_.size()-limit;while(records_.size()>limit)records_.pop_front();return removed;}

void CaptureReplayBuffer::clear(){records_.clear();}

}  // namespace aegis::detail
