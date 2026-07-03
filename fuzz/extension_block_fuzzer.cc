#include "aegis/aegis.hpp"
#include <cstddef>
#include <cstdint>
#include <span>

namespace {
aegis::Bytes u32(std::uint32_t value) {
    return {static_cast<std::uint8_t>(value >> 24U), static_cast<std::uint8_t>(value >> 16U),
            static_cast<std::uint8_t>(value >> 8U), static_cast<std::uint8_t>(value)};
}
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (size < 64 || size > 16 * 1024 * 1024) return 0;
    aegis::Engine engine;
    const auto open = aegis::make_frame(1, 11, {{1, u32(1)}, {2, u32(3)}, {3, {'r','o','u','t','e'}}});
    const auto stage = aegis::make_frame(3, 11, {{1, u32(2)}, {2, u32(3)}, {3, {'r','o','u','t','e'}},
                                                  {4, u32(2)}, {5, aegis::Bytes(data, data + size)}, {12, {1}}});
    const auto commit = aegis::make_frame(4, 11, {});
    const auto data_frame = aegis::make_frame(5, 11, {});
    (void)engine.ingest_capture(aegis::make_capture({open, stage, commit, data_frame}));
    return 0;
}
