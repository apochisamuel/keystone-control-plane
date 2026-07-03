#include "aegis/aegis.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
int failures = 0;
void expect(bool condition, const std::string& message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
aegis::Bytes u32(std::uint32_t value) {
    return {static_cast<std::uint8_t>(value >> 24U), static_cast<std::uint8_t>(value >> 16U),
            static_cast<std::uint8_t>(value >> 8U), static_cast<std::uint8_t>(value)};
}
void test_lifecycle() {
    aegis::Engine engine;
    const auto open = aegis::make_frame(1, 10, {{1, u32(1)}, {2, u32(4)}, {3, {'h','o','p'}}});
    const auto register_frame = aegis::make_frame(2, 10, {{14, {'s','u','b'}}});
    const auto stage = aegis::make_frame(3, 10, {{1, u32(2)}, {2, u32(4)}, {3, {'n','e','w'}}, {4, u32(2)}});
    const auto commit = aegis::make_frame(4, 10, {});
    const auto data = aegis::make_frame(5, 10, {});
    const auto result = engine.ingest_capture(aegis::make_capture({open, register_frame, stage, commit, data}));
    expect(result.accepted_frames == 5, "lifecycle accepts frames");
    expect(engine.stats().sessions == 1, "session counted");
    expect(engine.stats().policies == 1, "policy counted");
}
void test_rejects_bad_frame() {
    aegis::Engine engine;
    std::vector<std::uint8_t> malformed(64, 0xff);
    const auto result = engine.ingest_capture(malformed);
    expect(result.rejected_frames >= 1, "bad capture rejected");
}
void test_snapshot() {
    aegis::Engine engine;
    const auto open = aegis::make_frame(1, 20, {{1, u32(3)}, {2, u32(6)}, {3, {'s','n','a','p'}}});
    (void)engine.ingest_capture(open);
    const auto snapshot = engine.snapshot();
    aegis::Engine restored;
    const auto result = restored.restore(snapshot);
    expect(result.accepted_frames >= 1, "snapshot restored");
    expect(restored.stats().sessions == 1, "restored session counted");
}
void test_unknown_fields() {
    aegis::Engine engine;
    const auto open = aegis::make_frame(1, 30, {{1, u32(9)}, {2, u32(7)}, {3, {'h','o','p'}}, {200, {1,2,3,4}}});
    const auto result = engine.ingest_capture(open);
    expect(result.accepted_frames == 1, "unknown field retained");
}
void test_simulation_api() {
    aegis::SimulationConfig config;
    config.session_id = 44U;
    config.rounds = 3U;
    config.seed = 19U;
    config.minimum_payload = 12U;
    config.maximum_payload = 24U;
    const auto capture = aegis::make_synthetic_capture(config);
    expect(capture.size() >= 64U, "simulation capture has usable size");
    aegis::Engine engine;
    const auto result = engine.ingest_capture(capture);
    expect(result.rejected_frames == 0U, "simulation capture is accepted");
    expect(result.accepted_frames >= 10U, "simulation capture has full lifecycle");
    const auto report = aegis::run_synthetic_simulation(config);
    expect(report.find("Deterministic: yes") != std::string::npos, "simulation is deterministic");
}
}
int main() {
    test_lifecycle();
    test_rejects_bad_frame();
    test_snapshot();
    test_unknown_fields();
    test_simulation_api();
    if (failures != 0) return EXIT_FAILURE;
    std::cout << "aegis tests passed\n";
    return EXIT_SUCCESS;
}
