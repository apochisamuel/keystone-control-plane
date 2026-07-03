#include "internal.hpp"

namespace aegis {

struct Engine::Impl {
    detail::EngineCore core;
};

Engine::Engine() : impl_(std::make_unique<Impl>()) {}
Engine::~Engine() = default;
Engine::Engine(Engine&&) noexcept = default;
Engine& Engine::operator=(Engine&&) noexcept = default;

IngestResult Engine::ingest_capture(std::span<const std::uint8_t> data) {
    return impl_->core.ingest(data);
}

EngineStats Engine::stats() const { return impl_->core.stats(); }
std::string Engine::inspect() const { return impl_->core.inspect(); }
Bytes Engine::snapshot() const { return impl_->core.snapshot(); }
IngestResult Engine::restore(std::span<const std::uint8_t> data) { return impl_->core.restore(data); }
void Engine::reset() { impl_->core.reset(); }

Bytes make_frame(std::uint8_t operation, std::uint32_t session_id,
                 const std::vector<FieldInput>& fields, std::uint8_t flags) {
    detail::Frame frame;
    frame.operation = static_cast<detail::Operation>(operation);
    frame.flags = flags;
    frame.session_id = session_id;
    for (const auto& [tag, value] : fields) frame.fields.push_back(detail::Field{tag, value});
    return detail::write_frame(frame);
}

Bytes make_capture(const std::vector<Bytes>& frames) {
    Bytes result;
    for (const auto& frame : frames) result.insert(result.end(), frame.begin(), frame.end());
    return result;
}

Bytes make_synthetic_capture(const SimulationConfig& config) {
    detail::ControlPlaneSimulator simulator;
    return simulator.capture({config.session_id, config.rounds, config.seed, config.minimum_payload, config.maximum_payload});
}

std::string run_synthetic_simulation(const SimulationConfig& config) {
    detail::ControlPlaneSimulator simulator;
    return simulator.run({config.session_id, config.rounds, config.seed, config.minimum_payload, config.maximum_payload});
}

}  // namespace aegis
