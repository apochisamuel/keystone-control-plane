#include "aegis/aegis.hpp"
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (size < 64 || size > 16 * 1024 * 1024) return 0;
    aegis::Engine engine;
    std::vector<std::uint8_t> current(data, data + size);
    (void)engine.ingest_capture(current);
    const auto snapshot = engine.snapshot();
    (void)engine.restore(snapshot);
    return 0;
}
