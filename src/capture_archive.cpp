#include "internal.hpp"

namespace aegis::detail {

namespace {
constexpr std::array<Byte, 4> kArchiveMagic{{'A', 'G', 'A', 'R'}};
constexpr std::size_t kMaximumArchiveEntry = 16U * 1024U * 1024U;
}

void CaptureArchive::append(const PacketRecord& record) {
    const auto encoded = write_frame(record.frame);
    entries_.push_back({record.ordinal, record.frame.session_id, record.frame.operation,
                        record.checksum, encoded});
}

std::vector<ArchiveEntry> CaptureArchive::session(std::uint32_t session_id) const {
    std::vector<ArchiveEntry> result;
    for (const auto& entry : entries_) {
        if (entry.session_id == session_id) result.push_back(entry);
    }
    return result;
}

std::vector<ArchiveEntry> CaptureArchive::range(std::uint64_t first, std::uint64_t last) const {
    std::vector<ArchiveEntry> result;
    if (first > last) return result;
    for (const auto& entry : entries_) {
        if (entry.ordinal >= first && entry.ordinal <= last) result.push_back(entry);
    }
    return result;
}

Bytes CaptureArchive::encode() const {
    Writer writer;
    writer.bytes(kArchiveMagic);
    writer.u32(static_cast<std::uint32_t>(entries_.size()));
    for (const auto& entry : entries_) {
        writer.u64(entry.ordinal);
        writer.u32(entry.session_id);
        writer.u8(static_cast<Byte>(entry.operation));
        writer.u32(entry.checksum);
        writer.u32(static_cast<std::uint32_t>(entry.encoded.size()));
        writer.bytes(entry.encoded);
    }
    return writer.take();
}

std::vector<ArchiveEntry> CaptureArchive::decode(std::span<const Byte> bytes,
                                                 std::vector<std::string>& errors) const {
    std::vector<ArchiveEntry> result;
    try {
        Cursor cursor(bytes);
        if (cursor.remaining() < kArchiveMagic.size() + 4U) throw DecodeError("archive header truncated");
        for (const auto expected : kArchiveMagic) {
            if (cursor.u8() != expected) throw DecodeError("archive magic mismatch");
        }
        const auto count = cursor.u32();
        if (count > 1'000'000U) throw DecodeError("archive entry count exceeds limit");
        result.reserve(count);
        for (std::uint32_t index = 0; index < count; ++index) {
            const auto ordinal = cursor.u64();
            const auto session_id = cursor.u32();
            const auto operation = static_cast<Operation>(cursor.u8());
            const auto checksum = cursor.u32();
            const auto size = cursor.u32();
            if (size > kMaximumArchiveEntry || size > cursor.remaining()) throw DecodeError("archive entry length invalid");
            auto encoded = cursor.bytes(size);
            std::vector<std::string> frame_errors;
            const auto parsed = parse_capture(encoded, frame_errors);
            if (!frame_errors.empty() || parsed.size() != 1U) throw DecodeError("archive payload is not one frame");
            if (parsed.front().operation != operation || parsed.front().session_id != session_id) throw DecodeError("archive metadata mismatch");
            if (checksum32(encoded) != checksum) throw DecodeError("archive checksum mismatch");
            result.push_back({ordinal, session_id, operation, checksum, std::move(encoded)});
        }
        if (!cursor.empty()) errors.push_back("archive has trailing bytes");
    } catch (const DecodeError& error) {
        errors.push_back(error.what());
        result.clear();
    }
    return result;
}

std::size_t CaptureArchive::compact(std::size_t keep_per_session) {
    if (keep_per_session == 0U) {
        const auto removed = entries_.size();
        entries_.clear();
        return removed;
    }
    std::unordered_map<std::uint32_t, std::size_t> kept;
    std::vector<ArchiveEntry> retained;
    retained.reserve(entries_.size());
    for (auto iterator = entries_.rbegin(); iterator != entries_.rend(); ++iterator) {
        auto& count = kept[iterator->session_id];
        if (count++ < keep_per_session) retained.push_back(*iterator);
    }
    std::reverse(retained.begin(), retained.end());
    const auto removed = entries_.size() - retained.size();
    entries_ = std::move(retained);
    return removed;
}

std::string CaptureArchive::report() const {
    std::map<std::uint32_t, std::size_t> per_session;
    std::map<Operation, std::size_t> per_operation;
    std::size_t encoded_bytes{};
    for (const auto& entry : entries_) {
        ++per_session[entry.session_id];
        ++per_operation[entry.operation];
        encoded_bytes += entry.encoded.size();
    }
    std::vector<std::vector<std::string>> rows{{"metric", "value"}};
    rows.push_back({"entries", std::to_string(entries_.size())});
    rows.push_back({"encoded_bytes", std::to_string(encoded_bytes)});
    for (const auto& [session_id, count] : per_session) rows.push_back({"session." + std::to_string(session_id), std::to_string(count)});
    for (const auto& [operation, count] : per_operation) rows.push_back({"operation." + describe_operation(operation), std::to_string(count)});
    return render_table(rows);
}

void CaptureArchive::clear() { entries_.clear(); }

}  // namespace aegis::detail
