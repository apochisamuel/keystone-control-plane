#include "aegis/aegis.hpp"
#include <iostream>
namespace { aegis::Bytes u32(std::uint32_t x) { return {static_cast<std::uint8_t>(x>>24U),static_cast<std::uint8_t>(x>>16U),static_cast<std::uint8_t>(x>>8U),static_cast<std::uint8_t>(x)}; } }
int main() {
    aegis::Engine engine;
    const auto open = aegis::make_frame(1, 2, {{1,u32(1)},{2,u32(1)},{3,{'h','o','p'}}});
    const auto stage = aegis::make_frame(3, 2, {{1,u32(2)},{2,u32(3)},{3,{'p','o','l','i','c','y'}},{4,u32(2)}});
    const auto commit = aegis::make_frame(4, 2, {});
    const auto result = engine.ingest_capture(aegis::make_capture({open,stage,commit}));
    std::cout << "accepted=" << result.accepted_frames << " policies=" << engine.stats().policies << "\n";
    return result.rejected_frames == 0 ? 0 : 1;
}
