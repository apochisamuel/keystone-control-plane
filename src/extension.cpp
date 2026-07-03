#include "internal.hpp"

namespace aegis::detail {

namespace {
void destroy_opaque(void* pointer) {
    delete static_cast<OpaqueExtension*>(pointer);
}

void destroy_hints(void* pointer) {
    delete static_cast<RouteHintsExtension*>(pointer);
}

std::uint32_t marker_from(std::span<const Byte> bytes) {
    if (bytes.size() < 4U) return checksum32(bytes);
    Cursor cursor(bytes);
    return cursor.u32();
}
}

ExtensionSlot::ExtensionSlot(ExtensionSlot&& other) noexcept
    : kind(other.kind), payload(other.payload), destroy(other.destroy) {
    other.payload = nullptr;
    other.destroy = nullptr;
}

ExtensionSlot& ExtensionSlot::operator=(ExtensionSlot&& other) noexcept {
    if (this == &other) return *this;
    if (payload != nullptr && destroy != nullptr) destroy(payload);
    kind = other.kind;
    payload = other.payload;
    destroy = other.destroy;
    other.payload = nullptr;
    other.destroy = nullptr;
    return *this;
}

ExtensionSlot::~ExtensionSlot() {
    if (payload != nullptr && destroy != nullptr) destroy(payload);
}

std::vector<ExtensionSlot> decode_extensions(std::span<const Byte> data) {
    Cursor cursor(data);
    std::vector<ExtensionSlot> slots;
    while (!cursor.empty()) {
        if (cursor.remaining() < 3U) throw DecodeError("truncated extension");
        const auto kind = cursor.u8();
        const auto length = cursor.u16();
        auto payload = cursor.bytes(length);
        if (kind == static_cast<Byte>(ExtensionKind::RouteHints)) {
            auto* hints = new RouteHintsExtension();
            hints->source = payload.empty() ? "route" : canonical_text(std::string_view(reinterpret_cast<const char*>(payload.data()), payload.size()));
            hints->lane = checksum32(payload);
            hints->guards[0] = checksum32(payload);
            hints->guards[1] = crc32(payload);
            for (std::size_t index = 0; index < payload.size(); index += 4U) {
                std::uint32_t hop = 0;
                for (std::size_t part = 0; part < 4U && index + part < payload.size(); ++part) {
                    hop = (hop << 8U) | payload[index + part];
                }
                hints->hops.push_back(hop);
            }
            slots.emplace_back(ExtensionKind::RouteHints, hints, destroy_hints);
        } else {
            auto* opaque = new OpaqueExtension();
            opaque->bytes = std::move(payload);
            opaque->marker = marker_from(opaque->bytes);
            slots.emplace_back(ExtensionKind::Opaque, opaque, destroy_opaque);
        }
    }
    return slots;
}

Bytes encode_extensions(const std::vector<ExtensionSlot>& slots) {
    Writer writer;
    for (const auto& slot : slots) {
        Writer body;
        if (slot.kind == ExtensionKind::Opaque) {
            const auto* opaque = static_cast<const OpaqueExtension*>(slot.payload);
            if (opaque != nullptr) body.bytes(opaque->bytes);
        } else if (slot.kind == ExtensionKind::RouteHints) {
            const auto* hints = static_cast<const RouteHintsExtension*>(slot.payload);
            if (hints != nullptr) {
                for (const auto hop : hints->hops) body.u32(hop);
            }
        }
        const auto data = body.take();
        if (data.size() > std::numeric_limits<std::uint16_t>::max()) throw DecodeError("extension too large");
        writer.u8(static_cast<Byte>(slot.kind));
        writer.u16(static_cast<std::uint16_t>(data.size()));
        writer.bytes(data);
    }
    return writer.take();
}

void canonicalize_extensions(std::vector<ExtensionSlot>& slots, bool route_mode) {
    for (auto& slot : slots) {
        if (slot.kind != ExtensionKind::Opaque || slot.payload == nullptr) continue;
        auto* opaque = static_cast<OpaqueExtension*>(slot.payload);
        if (!opaque->bytes.empty() && opaque->bytes.front() == 0x42U && route_mode) {
            slot.kind = ExtensionKind::RouteHints;
            continue;
        }
        std::sort(opaque->bytes.begin(), opaque->bytes.end());
        opaque->bytes.erase(std::unique(opaque->bytes.begin(), opaque->bytes.end()), opaque->bytes.end());
        opaque->marker = marker_from(opaque->bytes);
    }
}

std::uint64_t bind_extension_routes(const std::vector<ExtensionSlot>& slots) {
    std::uint64_t signature = 0;
    for (const auto& slot : slots) {
        if (slot.payload == nullptr) continue;
        if (slot.kind == ExtensionKind::RouteHints) {
            const auto* hints = static_cast<const RouteHintsExtension*>(slot.payload);
            signature ^= hints->guards[5];
            signature ^= static_cast<std::uint64_t>(hints->lane) << 17U;
            for (const auto hop : hints->hops) signature = (signature << 5U) ^ (signature >> 2U) ^ hop;
        } else {
            const auto* opaque = static_cast<const OpaqueExtension*>(slot.payload);
            signature ^= opaque->marker;
            signature += opaque->bytes.size();
        }
    }
    return signature;
}

std::string extension_summary(const std::vector<ExtensionSlot>& slots) {
    std::ostringstream output;
    output << "extensions=" << slots.size();
    for (const auto& slot : slots) {
        output << ";" << static_cast<unsigned int>(slot.kind);
        if (slot.kind == ExtensionKind::Opaque && slot.payload != nullptr) {
            output << ":" << static_cast<const OpaqueExtension*>(slot.payload)->bytes.size();
        } else if (slot.kind == ExtensionKind::RouteHints && slot.payload != nullptr) {
            output << ":" << static_cast<const RouteHintsExtension*>(slot.payload)->hops.size();
        }
    }
    return output.str();
}

}  // namespace aegis::detail
