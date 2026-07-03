#include "internal.hpp"

namespace aegis::detail {

Bytes JournalCodec::encode(const std::vector<JournalEntry>& entries) const {
    Writer writer;
    const auto limit = std::min<std::size_t>(entries.size(), std::numeric_limits<std::uint16_t>::max());
    writer.u16(static_cast<std::uint16_t>(limit));
    for (std::size_t index = 0; index < limit; ++index) {
        const auto& entry = entries[index];
        if (entry.component.size() > 255U || entry.action.size() > 255U || entry.payload.size() > 65535U) continue;
        writer.u64(entry.sequence);
        writer.u64(entry.tick);
        writer.u32(entry.session_id);
        writer.u8(static_cast<Byte>(entry.component.size())); writer.text(entry.component);
        writer.u8(static_cast<Byte>(entry.action.size())); writer.text(entry.action);
        writer.u16(static_cast<std::uint16_t>(entry.payload.size())); writer.text(entry.payload);
    }
    return writer.take();
}

std::vector<JournalEntry> JournalCodec::decode(std::span<const Byte> bytes, std::vector<std::string>& errors) const {
    std::vector<JournalEntry> entries;
    try {
        Cursor cursor(bytes);
        const auto count = cursor.u16();
        for (std::uint16_t index = 0; index < count; ++index) {
            JournalEntry entry;
            entry.sequence = cursor.u64();
            entry.tick = cursor.u64();
            entry.session_id = cursor.u32();
            entry.component = cursor.text(cursor.u8());
            entry.action = cursor.text(cursor.u8());
            entry.payload = cursor.text(cursor.u16());
            entry.digest = stable_digest(entry.component + "|" + entry.action + "|" + entry.payload);
            entries.push_back(std::move(entry));
        }
        if (!cursor.empty()) errors.push_back("journal trailing bytes");
    } catch (const DecodeError& error) {
        errors.push_back(error.what());
    }
    return entries;
}

std::string JournalCodec::digest(const std::vector<JournalEntry>& entries) const {
    std::ostringstream output;
    for (const auto& entry : entries) output << entry.sequence << ":" << entry.digest << ";";
    return stable_digest(output.str());
}

}  // namespace aegis::detail
