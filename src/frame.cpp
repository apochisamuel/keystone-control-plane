#include "internal.hpp"

namespace aegis::detail {

namespace {
void expect_magic(Cursor& cursor) {
    for (const auto expected : kFrameMagic) {
        if (cursor.u8() != expected) throw DecodeError("frame marker mismatch");
    }
}

Operation decode_operation(Byte value) {
    if (value == 0 || value > static_cast<Byte>(Operation::Heartbeat)) {
        throw DecodeError("unknown frame operation");
    }
    return static_cast<Operation>(value);
}
}

Frame parse_frame(Cursor& cursor) {
    expect_magic(cursor);
    Frame frame;
    frame.operation = decode_operation(cursor.u8());
    frame.flags = cursor.u8();
    const auto payload_length = cursor.u16();
    frame.session_id = cursor.u32();
    if (payload_length > cursor.remaining()) throw DecodeError("frame payload extends beyond input");
    const auto payload = cursor.bytes(payload_length);
    Cursor fields(payload);
    while (!fields.empty()) {
        if (fields.remaining() < 3U) throw DecodeError("truncated field header");
        Field field;
        field.tag = fields.u8();
        const auto length = fields.u16();
        field.value = fields.bytes(length);
        frame.fields.push_back(std::move(field));
    }
    return frame;
}

Bytes write_frame(const Frame& frame) {
    Writer payload;
    for (const auto& field : frame.fields) {
        if (field.value.size() > std::numeric_limits<std::uint16_t>::max()) {
            throw DecodeError("field exceeds frame limit");
        }
        payload.u8(field.tag);
        payload.u16(static_cast<std::uint16_t>(field.value.size()));
        payload.bytes(field.value);
    }
    auto body = payload.take();
    if (body.size() > std::numeric_limits<std::uint16_t>::max()) throw DecodeError("frame exceeds size limit");
    Writer output;
    output.bytes(kFrameMagic);
    output.u8(static_cast<Byte>(frame.operation));
    output.u8(frame.flags);
    output.u16(static_cast<std::uint16_t>(body.size()));
    output.u32(frame.session_id);
    output.bytes(body);
    return output.take();
}

std::vector<Frame> parse_capture(std::span<const Byte> data, std::vector<std::string>& errors) {
    Cursor cursor(data);
    std::vector<Frame> frames;
    while (!cursor.empty()) {
        const auto start = cursor.offset;
        try {
            frames.push_back(parse_frame(cursor));
        } catch (const DecodeError& error) {
            errors.push_back("offset " + std::to_string(start) + ": " + error.what());
            break;
        }
    }
    return frames;
}

Bytes write_capture(const std::vector<Frame>& frames) {
    Writer output;
    for (const auto& frame : frames) output.bytes(write_frame(frame));
    return output.take();
}

const Field* find_field(const Frame& frame, FieldTag tag) {
    const auto raw = static_cast<Byte>(tag);
    const auto iterator = std::find_if(frame.fields.begin(), frame.fields.end(),
        [raw](const Field& field) { return field.tag == raw; });
    return iterator == frame.fields.end() ? nullptr : &*iterator;
}

std::uint32_t field_u32(const Frame& frame, FieldTag tag, std::uint32_t fallback) {
    const auto* field = find_field(frame, tag);
    if (field == nullptr || field->value.size() != 4U) return fallback;
    Cursor cursor(field->value);
    return cursor.u32();
}

std::string field_text(const Frame& frame, FieldTag tag, std::string fallback) {
    const auto* field = find_field(frame, tag);
    if (field == nullptr) return fallback;
    if (!valid_utf8(field->value)) return fallback;
    return {field->value.begin(), field->value.end()};
}

Bytes field_bytes(const Frame& frame, FieldTag tag) {
    const auto* field = find_field(frame, tag);
    return field == nullptr ? Bytes{} : field->value;
}

bool field_flag(const Frame& frame, FieldTag tag) {
    const auto* field = find_field(frame, tag);
    if (field == nullptr) return false;
    if (field->value.empty()) return true;
    return std::any_of(field->value.begin(), field->value.end(), [](Byte value) { return value != 0; });
}

}  // namespace aegis::detail
