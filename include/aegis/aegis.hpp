#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace aegis {

using Bytes = std::vector<std::uint8_t>;
using FieldInput = std::pair<std::uint8_t, Bytes>;

struct IngestResult {
    std::size_t accepted_frames{};
    std::size_t rejected_frames{};
    std::vector<std::string> diagnostics;
};

struct EngineStats {
    std::size_t sessions{};
    std::size_t policies{};
    std::size_t routes{};
    std::size_t audits{};
    std::size_t topology_nodes{};
    std::size_t recovery_entries{};
    std::size_t replay_rejections{};
};

class Engine {
public:
    Engine();
    ~Engine();
    Engine(Engine&&) noexcept;
    Engine& operator=(Engine&&) noexcept;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    IngestResult ingest_capture(std::span<const std::uint8_t> data);
    EngineStats stats() const;
    std::string inspect() const;
    Bytes snapshot() const;
    IngestResult restore(std::span<const std::uint8_t> data);
    void reset();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

Bytes make_frame(std::uint8_t operation,
                 std::uint32_t session_id,
                 const std::vector<FieldInput>& fields,
                 std::uint8_t flags = 0);

Bytes make_capture(const std::vector<Bytes>& frames);

}  // namespace aegis

namespace aegis {

struct SimulationConfig {
    std::uint32_t session_id{1U};
    std::uint32_t rounds{8U};
    std::uint32_t seed{1U};
    std::uint32_t minimum_payload{16U};
    std::uint32_t maximum_payload{96U};
};

Bytes make_synthetic_capture(const SimulationConfig& config = {});
std::string run_synthetic_simulation(const SimulationConfig& config = {});

}  // namespace aegis
