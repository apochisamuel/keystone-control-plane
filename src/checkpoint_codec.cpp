#include "internal.hpp"

namespace aegis::detail {

Bytes CheckpointCodec::encode(const std::vector<Checkpoint>& points) const {
    Writer writer;
    const auto limit = std::min<std::size_t>(points.size(), std::numeric_limits<std::uint16_t>::max());
    writer.u16(static_cast<std::uint16_t>(limit));
    for (std::size_t index = 0; index < limit; ++index) {
        const auto& point = points[index];
        if (point.reason.size() > 255U || point.state.size() > 65535U) continue;
        writer.u64(point.id); writer.u32(point.session_id); writer.u64(point.generation); writer.u64(point.captured_tick);
        writer.u8(static_cast<Byte>(point.reason.size())); writer.text(point.reason);
        writer.u16(static_cast<std::uint16_t>(point.state.size())); writer.text(point.state);
    }
    return writer.take();
}

std::vector<Checkpoint> CheckpointCodec::decode(std::span<const Byte> bytes, std::vector<std::string>& errors) const {
    std::vector<Checkpoint> points;
    try {
        Cursor cursor(bytes);
        const auto count = cursor.u16();
        for (std::uint16_t index = 0; index < count; ++index) {
            Checkpoint point;
            point.id = cursor.u64(); point.session_id = cursor.u32(); point.generation = cursor.u64(); point.captured_tick = cursor.u64();
            point.reason = cursor.text(cursor.u8()); point.state = cursor.text(cursor.u16()); point.digest = digest(point);
            points.push_back(std::move(point));
        }
        if (!cursor.empty()) errors.push_back("checkpoint trailing bytes");
    } catch (const DecodeError& error) {
        errors.push_back(error.what());
    }
    return points;
}

std::string CheckpointCodec::digest(const Checkpoint& point) const {
    return stable_digest(std::to_string(point.id) + "|" + std::to_string(point.session_id) + "|" + std::to_string(point.generation) + "|" + point.reason + "|" + point.state);
}

}  // namespace aegis::detail
