#include "aegis/aegis.hpp"
#include <iostream>
namespace { aegis::Bytes u32(std::uint32_t x) { return {static_cast<std::uint8_t>(x>>24U),static_cast<std::uint8_t>(x>>16U),static_cast<std::uint8_t>(x>>8U),static_cast<std::uint8_t>(x)}; } }
int main() {
    aegis::Engine engine;
    const auto open = aegis::make_frame(1, 5, {{1,u32(1)},{2,u32(2)},{3,{'r','e','c'}}});
    const auto handover = aegis::make_frame(6, 5, {{17,u32(99)}});
    const auto recover = aegis::make_frame(12, 5, {{11,u32(0)}});
    const auto result = engine.ingest_capture(aegis::make_capture({open,handover,recover}));
    std::cout << "accepted=" << result.accepted_frames << " recovery=" << engine.stats().recovery_entries << "\n";
    return result.rejected_frames == 0 ? 0 : 1;
}
