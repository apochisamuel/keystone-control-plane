#include "internal.hpp"

namespace aegis::detail {

std::size_t Cursor::remaining() const {
    return offset <= input.size() ? input.size() - offset : 0;
}

bool Cursor::empty() const {
    return remaining() == 0;
}

Byte Cursor::u8() {
    if (remaining() < 1) throw DecodeError("unexpected end of input");
    return input[offset++];
}

std::uint16_t Cursor::u16() {
    if (remaining() < 2) throw DecodeError("unexpected end of input");
    const auto value = static_cast<std::uint16_t>((static_cast<std::uint16_t>(input[offset]) << 8U) |
                                                  static_cast<std::uint16_t>(input[offset + 1]));
    offset += 2;
    return value;
}

std::uint32_t Cursor::u32() {
    if (remaining() < 4) throw DecodeError("unexpected end of input");
    std::uint32_t value = 0;
    for (int index = 0; index < 4; ++index) {
        value = (value << 8U) | input[offset + static_cast<std::size_t>(index)];
    }
    offset += 4;
    return value;
}

std::uint64_t Cursor::u64() {
    if (remaining() < 8) throw DecodeError("unexpected end of input");
    std::uint64_t value = 0;
    for (int index = 0; index < 8; ++index) {
        value = (value << 8U) | input[offset + static_cast<std::size_t>(index)];
    }
    offset += 8;
    return value;
}

Bytes Cursor::bytes(std::size_t count) {
    if (remaining() < count) throw DecodeError("truncated byte range");
    Bytes result(input.begin() + static_cast<std::ptrdiff_t>(offset),
                 input.begin() + static_cast<std::ptrdiff_t>(offset + count));
    offset += count;
    return result;
}

std::string Cursor::text(std::size_t count) {
    const auto raw = bytes(count);
    return {raw.begin(), raw.end()};
}

void Cursor::skip(std::size_t count) {
    if (remaining() < count) throw DecodeError("truncated skip");
    offset += count;
}

void Writer::u8(Byte value) {
    output.push_back(value);
}

void Writer::u16(std::uint16_t value) {
    output.push_back(static_cast<Byte>((value >> 8U) & 0xffU));
    output.push_back(static_cast<Byte>(value & 0xffU));
}

void Writer::u32(std::uint32_t value) {
    for (int shift = 24; shift >= 0; shift -= 8) {
        output.push_back(static_cast<Byte>((value >> static_cast<std::uint32_t>(shift)) & 0xffU));
    }
}

void Writer::u64(std::uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8) {
        output.push_back(static_cast<Byte>((value >> static_cast<std::uint64_t>(shift)) & 0xffU));
    }
}

void Writer::bytes(std::span<const Byte> value) {
    output.insert(output.end(), value.begin(), value.end());
}

void Writer::text(std::string_view value) {
    output.insert(output.end(), value.begin(), value.end());
}

Bytes Writer::take() {
    return std::move(output);
}

std::uint32_t crc32(std::span<const Byte> data) {
    std::uint32_t value = 0xffffffffU;
    for (const auto byte : data) {
        value ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            const auto mask = static_cast<std::uint32_t>(-(static_cast<std::int32_t>(value & 1U)));
            value = (value >> 1U) ^ (0xedb88320U & mask);
        }
    }
    return ~value;
}

std::uint32_t checksum32(std::span<const Byte> data) {
    std::uint32_t state = 2166136261U;
    for (const auto byte : data) {
        state ^= byte;
        state *= 16777619U;
        state ^= state >> 13U;
    }
    state ^= state << 7U;
    return state;
}

Bytes encode_varint(std::uint64_t value) {
    Bytes encoded;
    do {
        Byte out = static_cast<Byte>(value & 0x7fU);
        value >>= 7U;
        if (value != 0) out = static_cast<Byte>(out | 0x80U);
        encoded.push_back(out);
    } while (value != 0);
    return encoded;
}

std::uint64_t decode_varint(Cursor& cursor) {
    std::uint64_t value = 0;
    for (unsigned int shift = 0; shift < 64; shift += 7) {
        const auto byte = cursor.u8();
        value |= static_cast<std::uint64_t>(byte & 0x7fU) << shift;
        if ((byte & 0x80U) == 0) return value;
    }
    throw DecodeError("overlong varint");
}

std::string hex(std::span<const Byte> data) {
    static constexpr char alphabet[] = "0123456789abcdef";
    std::string result;
    result.reserve(data.size() * 2);
    for (const auto value : data) {
        result.push_back(alphabet[(value >> 4U) & 0x0fU]);
        result.push_back(alphabet[value & 0x0fU]);
    }
    return result;
}

namespace {
int nibble(char value) {
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
}
}

Bytes unhex(std::string_view text) {
    if ((text.size() % 2U) != 0U) throw DecodeError("odd hexadecimal length");
    Bytes result;
    result.reserve(text.size() / 2U);
    for (std::size_t index = 0; index < text.size(); index += 2U) {
        const auto high = nibble(text[index]);
        const auto low = nibble(text[index + 1]);
        if (high < 0 || low < 0) throw DecodeError("invalid hexadecimal digit");
        result.push_back(static_cast<Byte>((high << 4) | low));
    }
    return result;
}

bool valid_utf8(std::span<const Byte> data) {
    std::size_t index = 0;
    while (index < data.size()) {
        const Byte head = data[index++];
        if (head < 0x80U) continue;
        std::size_t trailing = 0;
        std::uint32_t codepoint = 0;
        if ((head & 0xe0U) == 0xc0U) { trailing = 1; codepoint = head & 0x1fU; }
        else if ((head & 0xf0U) == 0xe0U) { trailing = 2; codepoint = head & 0x0fU; }
        else if ((head & 0xf8U) == 0xf0U) { trailing = 3; codepoint = head & 0x07U; }
        else return false;
        if (index + trailing > data.size()) return false;
        for (std::size_t part = 0; part < trailing; ++part) {
            const Byte next = data[index++];
            if ((next & 0xc0U) != 0x80U) return false;
            codepoint = (codepoint << 6U) | (next & 0x3fU);
        }
        if ((trailing == 1 && codepoint < 0x80U) ||
            (trailing == 2 && codepoint < 0x800U) ||
            (trailing == 3 && codepoint < 0x10000U) ||
            codepoint > 0x10ffffU || (codepoint >= 0xd800U && codepoint <= 0xdfffU)) return false;
    }
    return true;
}

std::string escaped_json(std::string_view value) {
    std::ostringstream output;
    for (const unsigned char byte : value) {
        switch (byte) {
            case '"': output << "\\\""; break;
            case '\\': output << "\\\\"; break;
            case '\b': output << "\\b"; break;
            case '\f': output << "\\f"; break;
            case '\n': output << "\\n"; break;
            case '\r': output << "\\r"; break;
            case '\t': output << "\\t"; break;
            default:
                if (byte < 0x20U) {
                    output << "\\u00" << std::hex << std::setw(2) << std::setfill('0')
                           << static_cast<unsigned int>(byte) << std::dec << std::setfill(' ');
                } else {
                    output << static_cast<char>(byte);
                }
        }
    }
    return output.str();
}

std::string canonical_text(std::string_view value) {
    std::string result;
    result.reserve(value.size());
    bool previous_space = false;
    for (const unsigned char byte : value) {
        if (byte <= 0x20U) {
            if (!result.empty() && !previous_space) result.push_back(' ');
            previous_space = true;
            continue;
        }
        previous_space = false;
        if (byte >= 'A' && byte <= 'Z') result.push_back(static_cast<char>(byte - 'A' + 'a'));
        else result.push_back(static_cast<char>(byte));
    }
    while (!result.empty() && result.back() == ' ') result.pop_back();
    return result;
}

std::vector<std::string> split_labels(std::string_view value) {
    std::vector<std::string> labels;
    std::string current;
    for (const char ch : value) {
        if (ch == ',' || ch == ';' || ch == '|') {
            const auto normalized = canonical_text(current);
            if (!normalized.empty()) labels.push_back(normalized);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    const auto normalized = canonical_text(current);
    if (!normalized.empty()) labels.push_back(normalized);
    std::sort(labels.begin(), labels.end());
    labels.erase(std::unique(labels.begin(), labels.end()), labels.end());
    return labels;
}

std::string join_labels(const std::vector<std::string>& labels, std::string_view separator) {
    std::ostringstream output;
    for (std::size_t index = 0; index < labels.size(); ++index) {
        if (index != 0) output << separator;
        output << labels[index];
    }
    return output.str();
}

}  // namespace aegis::detail
