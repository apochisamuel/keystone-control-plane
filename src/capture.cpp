#include "internal.hpp"

namespace aegis::detail {

void CaptureIndex::add(const PacketRecord& record) {
    offsets_[record.frame.session_id].push_back(record.ordinal);
}

std::vector<std::uint64_t> CaptureIndex::session_offsets(std::uint32_t session_id) const {
    const auto iterator = offsets_.find(session_id);
    return iterator == offsets_.end() ? std::vector<std::uint64_t>{} : iterator->second;
}

std::size_t CaptureIndex::size() const {
    std::size_t count = 0;
    for (const auto& [session, offsets] : offsets_) {
        (void)session;
        count += offsets.size();
    }
    return count;
}

void CaptureIndex::clear() {
    offsets_.clear();
}

StreamWindow::StreamWindow(std::size_t limit) : limit_(std::max<std::size_t>(1, limit)) {}

void StreamWindow::push(PacketRecord record) {
    records_.push_back(std::move(record));
    while (records_.size() > limit_) records_.pop_front();
}

std::vector<PacketRecord> StreamWindow::recent(std::size_t count) const {
    if (count >= records_.size()) return {records_.begin(), records_.end()};
    return {records_.end() - static_cast<std::ptrdiff_t>(count), records_.end()};
}

std::size_t StreamWindow::size() const {
    return records_.size();
}

void StreamWindow::clear() {
    records_.clear();
}

void FragmentBuffer::append(std::uint32_t stream_id, std::span<const Byte> data) {
    auto& target = fragments_[stream_id];
    if (target.size() + data.size() > 1024U * 1024U) {
        target.clear();
        throw DecodeError("fragment buffer limit exceeded");
    }
    target.insert(target.end(), data.begin(), data.end());
}

std::optional<Bytes> FragmentBuffer::complete(std::uint32_t stream_id, std::size_t expected) {
    const auto iterator = fragments_.find(stream_id);
    if (iterator == fragments_.end() || iterator->second.size() < expected) return std::nullopt;
    Bytes result(iterator->second.begin(), iterator->second.begin() + static_cast<std::ptrdiff_t>(expected));
    if (iterator->second.size() == expected) fragments_.erase(iterator);
    else iterator->second.erase(iterator->second.begin(), iterator->second.begin() + static_cast<std::ptrdiff_t>(expected));
    return result;
}

void FragmentBuffer::discard(std::uint32_t stream_id) {
    fragments_.erase(stream_id);
}

std::size_t FragmentBuffer::buffered() const {
    std::size_t total = 0;
    for (const auto& [stream, data] : fragments_) {
        (void)stream;
        total += data.size();
    }
    return total;
}

}  // namespace aegis::detail
